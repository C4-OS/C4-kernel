#ifndef _C4_SYNCRONIZATION_H
#define _C4_SYNCRONIZATION_H 1
#include <stdbool.h>
#include <stdint.h>
#include <c4/debug.h>
#include <c4/thread.h>
#include <c4/klib/string.h>

#include <stdatomic.h>

// simple mutex lock
//typedef bool lock_t;
typedef atomic_flag lock_t;

// builds on top of the simple lock to do smarter things on SMP systems
typedef struct c4_mutex {
	lock_t lock;
	// number of running threads waiting on this lock (spinlocks)
	uint32_t run_waiting;
	// threads that are sleeping on this lock
	thread_list_t sleep_waiting;
	// current thread holding the lock, if any
	thread_t *active;
} mutex_t;

static inline bool lock_trylock(lock_t *lock) {
	// atomic_flag_test_and_set returns true if the lock is currently locked,
	// so to return true on acquiring the lock, we negate the result
	return !atomic_flag_test_and_set(lock);
}

static inline void lock_spinlock(lock_t *lock) {
	while (!lock_trylock(lock));
}

static inline void lock_unlock(lock_t *lock) {
	atomic_flag_clear(lock);
}

static inline void mutex_init(mutex_t *mutex) {
	memset(mutex, 0, sizeof(*mutex));
}

static inline void mutex_wait(mutex_t *mutex) {
	while (!lock_trylock(&mutex->lock)) {
		// TODO: thread_list functions aren't thread-safe, this is an issue
		//       if mutex_wait() is used on an SMP system
		thread_t *cur = sched_current_thread();
		debug_printf(" ! waiting for lock %p\n", &mutex->lock);

		thread_list_remove(&cur->sched);
		thread_list_insert(&mutex->sleep_waiting, &cur->sched);
		sched_thread_yield();
	}
}

/* NOTE: this function should only be called if there's a valid thread
 *       context for this thread, e.g. sched_current_thread() returns
 *       a valid result. Basically any time after threading is set up for the
 *       CPU.
 */
static inline void mutex_lock(mutex_t *mutex) {
	/* use an appropriate lock depending on whether there's multiple CPUs,
	 * since spinlocks will eat up CPU time until the next task switch on a
	 * non-SMP system, and sleeping when the lock is held by an active thread
	 * on an SMP system will usually have more overhead than spinning until
	 * the thread is finished.
	 */

	thread_t *cur = sched_current_thread();

	if (mutex->active == cur) {
		// this thread already owns the lock, no need to wait
		return;

	} else if (sched_num_cpus() > 1) {
		__atomic_add_fetch(&mutex->run_waiting, 1, __ATOMIC_SEQ_CST);
		lock_spinlock(&mutex->lock);
		__atomic_sub_fetch(&mutex->run_waiting, 1, __ATOMIC_SEQ_CST);

	} else {
		mutex_wait(mutex);
	}

	__atomic_store_n(&mutex->active, sched_current_thread(), __ATOMIC_SEQ_CST);
}

static inline void mutex_unlock(mutex_t *mutex) {
	thread_t *next_thread = NULL;

	if (atomic_load(&mutex->run_waiting) == 0) {
		// only try to wake up a thread if there's nothing spinning on the lock
		next_thread = thread_list_pop(&mutex->sleep_waiting);
	}

	thread_t *cur = sched_current_thread();

	KASSERT(cur == __atomic_exchange_n(&mutex->active, NULL, __ATOMIC_SEQ_CST));
	lock_unlock(&mutex->lock);

	if (next_thread) {
		sched_add_thread(next_thread);
	}
}

#endif
