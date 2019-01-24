#include <c4/scheduler.h>
#include <c4/klib/string.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/syncronization.h>

static lock_t sched_lock;
static thread_queue sched_list;

static thread_t *current_threads[SCHED_MAX_CPUS] = {NULL};
static thread_t *global_idle_threads[SCHED_MAX_CPUS] = {NULL};

static void idle_thread( void ){
	for (;;) {
		asm volatile ( "hlt" );
	}
}

void sched_init(void) {
	thread_queue_init(&sched_list);
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
		thread_queue_push_back(&sched_list, current_threads[cur_cpu]);
	}

	thread_t *next = thread_queue_pop_front(&sched_list);

	if (next){
		KASSERT(next->state == SCHED_STATE_RUNNING);
		KASSERT(FLAG(next, THREAD_FLAG_RUNNING) == false);

		if (next->state != SCHED_STATE_RUNNING || FLAG(next, THREAD_FLAG_RUNNING)) {
			debug_printf("/!\\ thread is already running or something!\n");
			debug_printf("    %p: state: %u, flags: %u, id: %u\n", next, next->state, next->flags, next->id);

			lock_unlock(&sched_lock);
			sched_jump_to_thread(global_idle_threads[cur_cpu]);
		}

		lock_unlock(&sched_lock);
		sched_jump_to_thread(next);

	} else {
		lock_unlock(&sched_lock);
		sched_jump_to_thread(global_idle_threads[cur_cpu]);
	}
}

void kernel_stack_set( void *addr );
void *kernel_stack_get( void );

void sched_jump_to_thread(thread_t *thread) {
	unsigned cur_cpu = sched_current_cpu();
	thread_t *cur = current_threads[cur_cpu];
	current_threads[cur_cpu] = thread;

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
		UNSET_FLAG(cur, THREAD_FLAG_RUNNING);

		// unlock currently running thread
		kobject_unlock(&cur->object);
	}

	// lock the thread while it's running
	kobject_lock(&thread->object);

	SET_FLAG(thread, THREAD_FLAG_RUNNING);
	kernel_stack_set(thread->kernel_stack);
	sched_do_thread_switch(cur, thread);
}

void sched_thread_yield( void ){
	sched_switch_thread( );
}

void sched_add_thread(thread_t *thread) {
	lock_spinlock(&sched_lock);

	// TODO: threads shouldn't be added if they're not already in running state
	KASSERT(thread->state == SCHED_STATE_RUNNING);
	if (thread->state == SCHED_STATE_RUNNING) {
		thread_queue_push_back(&sched_list, thread);
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
		thread_queue_push_back(&sched_list, thread);
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
