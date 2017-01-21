#ifndef _C4_SCHEDULER_H
#define _C4_SCHEDULER_H 1
#include <c4/thread.h>
#include <c4/arch/scheduler.h>

enum {
	SCHED_STATE_RUNNING,
	SCHED_STATE_STOPPED,
	SCHED_STATE_WAITING,
	SCHED_STATE_WAITING_ASYNC,
	SCHED_STATE_SENDING,
};

void init_scheduler( void );
void sched_switch_thread( void );
void sched_do_thread_switch( thread_t *cur, thread_t *next );
void sched_jump_to_thread( thread_t *thread );
void sched_add_thread( thread_t *thread );

void sched_thread_continue( thread_t *thread );
void sched_thread_stop( thread_t *thread );
void sched_thread_exit( void );
void sched_thread_kill( thread_t *thread );

thread_t *sched_current_thread( void );

#endif
