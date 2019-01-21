#ifndef _C4_SYNCRONIZATION_H
#define _C4_SYNCRONIZATION_H 1
#include <stdbool.h>
#include <stdint.h>
#include <c4/klib/string.h>

#include <stdatomic.h>

// simple mutex lock
typedef atomic_flag lock_t;

// builds on top of the simple lock to do smarter things on SMP systems
typedef struct c4_mutex {
	// number of running threads waiting on this lock (spinlocks)
	uint32_t run_waiting;

	// threads that are sleeping on this lock
	// TODO: use generic queue here
	//thread_list_t sleep_waiting;

	// current thread holding the lock, if any
	//thread_t *active;
	// XXX: can't include thread.h here, since this is part of kobject_t,
	//      which in turn is part of message_t, which is part of thread_t,
	//      which would be part of mutex_t here...
	// TODO: structs could use some work
	void *active;
	lock_t lock;
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

void mutex_wait(mutex_t *mutex);
void mutex_do_spinlock(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif
