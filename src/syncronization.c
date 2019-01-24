#include <c4/syncronization.h>
#include <c4/scheduler.h>
#include <c4/thread.h>
#include <c4/debug.h>

void mutex_wait(mutex_t *mutex) {
	while (!lock_trylock(&mutex->lock)) {
		// TODO: thread_list functions aren't thread-safe, this is an issue
		//       if mutex_wait() is used on an SMP system
		thread_t *cur = sched_current_thread();
		debug_printf(" ! waiting for lock %p\n", &mutex->lock);

        // TODO: use generic queue functions here
        cur->state = SCHED_STATE_WAITING;
        /*
		thread_list_remove(&cur->sched);
		thread_list_insert(&mutex->sleep_waiting, &cur->sched);
        */
		sched_thread_yield();
	}
}

void mutex_do_spinlock(mutex_t *mutex) {
	__atomic_add_fetch(&mutex->run_waiting, 1, __ATOMIC_SEQ_CST);
	lock_spinlock(&mutex->lock);
	__atomic_sub_fetch(&mutex->run_waiting, 1, __ATOMIC_SEQ_CST);
}

void mutex_lock(mutex_t *mutex) {
	/* use an appropriate lock depending on whether there's multiple CPUs,
	 * since spinlocks will eat up CPU time until the next task switch on a
	 * non-SMP system, and sleeping when the lock is held by an active thread
	 * on an SMP system will usually have more overhead than spinning until
	 * the thread is finished.
	 */

	thread_t *cur = sched_current_thread();

	if (!cur) {
		// if there's no current thread context, then we need to do a spinlock
		// since there's no way to suspend
		mutex_do_spinlock(mutex);

	} else if (mutex->active == cur) {
		// this thread already owns the lock, no need to wait
		return;

	} else if (sched_num_cpus() > 1) {
		mutex_do_spinlock(mutex);

	} else {
		mutex_wait(mutex);
	}

	__atomic_store_n(&mutex->active, sched_current_thread(), __ATOMIC_SEQ_CST);
}

void mutex_unlock(mutex_t *mutex) {
	thread_t *next_thread = NULL;

	if (atomic_load(&mutex->run_waiting) == 0) {
		// only try to wake up a thread if there's nothing spinning on the lock
        // TODO: generic queue
		//next_thread = thread_list_pop(&mutex->sleep_waiting);
	}

	thread_t *cur = sched_current_thread();
	lock_unlock(&mutex->lock);

	if (next_thread) {
        next_thread->state = SCHED_STATE_RUNNING;
		sched_add_thread(next_thread);
	}
}
