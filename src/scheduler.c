#include <c4/scheduler.h>
#include <c4/klib/string.h>
#include <c4/klib/andtree.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/syncronization.h>
#include <c4/timer.h>
#include <stdint.h>

static lock_t sched_lock;
static c4_atree_t sched_run_tree;

static thread_t *current_threads[SCHED_MAX_CPUS] = {NULL};
static thread_t *global_idle_threads[SCHED_MAX_CPUS] = {NULL};

static void idle_thread( void ){
	for (;;) {
		asm volatile ( "hlt" );
	}
}

static int64_t sched_thread_vruntime(void *b) {
	thread_t *thread = b;

	// simple virtual runtime, the vruntime is just the thread runtime in
	// nanoseconds divided by it's priority. definitely room for improvement,
	// but this is pretty flexible as-is.
	//
	// TODO: to adjust the thread's priority, we have to make sure the thread
	//       isn't in the run queue, otherwise it'll mess up the tree ordering.
	return thread->runtime / thread->priority;
}

void sched_init(void) {
	atree_init(&sched_run_tree, sched_thread_vruntime);

	memset(&current_threads, 0, sizeof(current_threads));
	memset(&global_idle_threads, 0, sizeof(global_idle_threads));
}

void sched_init_cpu(void) {
	unsigned cur_cpu = sched_current_cpu();

	global_idle_threads[cur_cpu] = thread_create_kthread(idle_thread);
	current_threads[cur_cpu] = NULL;
}

// TODO: rewrite scheduler to use a proper priority queue, move blocked
//       threads to seperate lists/queues/whatever
void sched_switch_thread( void ){
	lock_spinlock(&sched_lock);

	unsigned cur_cpu = sched_current_cpu();

	if (current_threads[cur_cpu]
	    && current_threads[cur_cpu] != global_idle_threads[cur_cpu]
	    && current_threads[cur_cpu]->state == SCHED_STATE_RUNNING)
	{
		UNSET_FLAG(current_threads[cur_cpu], THREAD_FLAG_RUNNING);
		atree_insert(&sched_run_tree, current_threads[cur_cpu]);
	}

	c4_anode_t *next_node = atree_start(&sched_run_tree);
	thread_t *next = global_idle_threads[cur_cpu];

	if (next_node) {
		next = next_node->data;
		atree_remove(&sched_run_tree, next_node);

		KASSERT(next != NULL);
		KASSERT(next->state == SCHED_STATE_RUNNING);
		KASSERT(FLAG(next, THREAD_FLAG_RUNNING) == false);

		if (next->state != SCHED_STATE_RUNNING || FLAG(next, THREAD_FLAG_RUNNING)) {
			debug_printf("/!\\ thread is already running or something!\n");
			debug_printf("    %p: state: %u, flags: %u, id: %u\n",
			             next, next->state, next->flags, next->id);
			next = global_idle_threads[cur_cpu];
		}
	}

	lock_unlock(&sched_lock);
	sched_jump_to_thread(next);
}

void kernel_stack_set( void *addr );
void *kernel_stack_get( void );

void sched_jump_to_thread(thread_t *thread) {
	unsigned cur_cpu = sched_current_cpu();
	thread_t *cur = current_threads[cur_cpu];
	current_threads[cur_cpu] = thread;

	if (cur == thread) {
		// we're already running this thread, nothing to do
		return;
	}

	if (!cur || thread->addr_space != cur->addr_space) {
		// XXX: cause a page fault if the address of the new page
		//      directory isn't mapped here, so it can be mapped in
		//      the page fault handler (and not cause a triple fault)
		volatile char *a = (char *)thread->addr_space->page_dir;
		volatile char  b = *a;
		// and keep compiler from complaining about used variable
		b = b;

		//set_page_dir( thread->addr_space->page_dir );
		addr_space_set(thread->addr_space);
	}

	// swap per-thread kernel stacks
	if (cur) {
		cur->kernel_stack = kernel_stack_get();
		cur->runtime += timer_get_timestamp_ns() - cur->start_timestamp;
		UNSET_FLAG(cur, THREAD_FLAG_RUNNING);
	}

	// lock the thread while it's running
	kobject_lock(&thread->object);

	SET_FLAG(thread, THREAD_FLAG_RUNNING);
	thread->start_timestamp = timer_get_timestamp_ns();

	kernel_stack_set(thread->kernel_stack);

	if (cur) {
		// unlock currently running thread
		kobject_unlock(&cur->object);
	}

	// TODO: set timer interrupt based on thread's priority, or disable the
	//       timer and wait for an interrupt if idle
	sched_do_thread_switch(cur, thread);
}

void sched_thread_yield( void ){
	sched_switch_thread( );
}

#include <c4/arch/apic.h>

void sched_add_thread(thread_t *thread) {
	lock_spinlock(&sched_lock);

	// TODO: threads shouldn't be added if they're not already in running state
	KASSERT(thread->state == SCHED_STATE_RUNNING);
	KASSERT(thread != NULL);

	if (thread->state == SCHED_STATE_RUNNING) {
		atree_insert(&sched_run_tree, thread);

		bool sent = false;

		// TODO: move this to x86-specific code
		if (apic_is_enabled()) {
			for (unsigned i = 0; i < SCHED_MAX_CPUS; i++) {
				if (global_idle_threads[i]
				    && current_threads[i] == global_idle_threads[i])
				{
					apic_send_ipi((void*)apic_get_base(),
					              APIC_IPI_FIXED | 32, i);
					sent = true;
					break;
				}
			}
		}

		if (!sent
		    && sched_current_thread()
		    && sched_thread_vruntime(sched_current_thread())
			< sched_thread_vruntime(thread))
		{
			// TODO: not this
			lock_unlock(&sched_lock);
			sched_thread_yield();
			return;
		}
	}

	lock_unlock(&sched_lock);
}

void sched_thread_continue(thread_t *thread) {
	lock_spinlock(&sched_lock);

	if (thread->state == SCHED_STATE_STOPPED) {
		thread->state = SCHED_STATE_RUNNING;

		// clear 'faulted' flag, assuming that if there was a fault,
		// the thread calling this function has resolved it
		UNSET_FLAG(thread, THREAD_FLAG_FAULTED);
		atree_insert(&sched_run_tree, thread);
	}

	lock_unlock(&sched_lock);
}

void sched_thread_stop( thread_t *thread ){
	if ( thread->state == SCHED_STATE_RUNNING ){
		thread->state = SCHED_STATE_STOPPED;
	}
}

void sched_thread_exit( void ){
	sched_thread_kill( sched_current_thread( ));
	sched_thread_yield( );

	for (;;);
}

void sched_thread_kill(thread_t *thread) {
	unsigned cur_cpu = sched_current_cpu();

	if (thread) {
		if (thread == current_threads[cur_cpu]) {
			current_threads[cur_cpu] = NULL;
		}

		// TODO: reclaim thread resources
		//       actually this will be done with garbage collection, can
		//       remove the thread_destroy() call here
		lock_spinlock(&sched_lock);
		sched_thread_stop(thread);
		// TODO: implement this properly, this doesn't actually make sure the
		//       thread is stopped
		lock_unlock(&sched_lock);
		thread_destroy(thread);
	}
}

thread_t *sched_current_thread( void ){
	return current_threads[sched_current_cpu()];
}
