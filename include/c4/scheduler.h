#ifndef _C4_SCHEDULER_H
#define _C4_SCHEDULER_H 1
#include <c4/thread.h>

enum {
	SCHED_FLAG_NONE,
	SCHED_FLAG_HAS_RAN,
	SCHED_FLAG_PENDING_MSG,
};

enum {
	SCHED_STATE_RUNNING,
	SCHED_STATE_WAITING,
	SCHED_STATE_SENDING,
};

void init_scheduler( void );
void sched_switch_thread( void );
void sched_jump_to_thread( thread_t *thread );
void sched_add_thread( thread_t *thread );
void sched_thread_exit( void );

thread_t *sched_get_thread_by_id( unsigned id );
thread_t *sched_current_thread( void );

#endif
