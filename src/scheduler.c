#include <c4/scheduler.h>
#include <c4/klib/string.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/syncronization.h>

static lock_t sched_lock;
static thread_list_t sched_list;

static thread_t *current_threads[SCHED_MAX_CPUS] = {NULL};
static thread_t *global_idle_threads[SCHED_MAX_CPUS] = {NULL};

static void idle_thread( void ){
	for (;;) {
		asm volatile ( "hlt" );
	}
}

void sched_init(void) {
	memset(&sched_list, 0, sizeof(thread_list_t));
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
		thread_list_insert(&sched_list, &current_threads[cur_cpu]->sched);
	}

	thread_node_t *asdf;
	for (asdf = sched_list.first; asdf && asdf->next; asdf = asdf->next);

	thread_t *next = asdf? asdf->thread : NULL;

	if (next){
		thread_list_remove(&next->sched);
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
	}

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
		thread_list_insert(&sched_list, &thread->sched);
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
		thread_list_insert(&sched_list, &thread->sched);
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
		thread_list_remove(&thread->sched);
		lock_unlock(&sched_lock);
		thread_destroy(thread);
	}
}

thread_t *sched_current_thread( void ){
	return current_threads[sched_current_cpu()];
}
