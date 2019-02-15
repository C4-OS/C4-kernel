#ifndef _C4_SCHEDULER_H
#define _C4_SCHEDULER_H 1
#include <c4/thread.h>
#include <c4/arch/scheduler.h>
#include <stdint.h>

// TODO: put this somewhere better, since this would be architecture-specific
enum {
	SCHED_MAX_CPUS = 32,
};

enum {
	SCHED_STATE_RUNNING,
	SCHED_STATE_STOPPED,
	SCHED_STATE_WAITING,
	SCHED_STATE_WAITING_ASYNC,
	SCHED_STATE_SENDING,
	SCHED_STATE_SLEEPING,
};

void sched_init(void);
void sched_init_cpu(void);
void sched_switch_thread(void);
void sched_do_thread_switch(thread_t *cur, thread_t *next);
void sched_jump_to_thread(thread_t *thread);
void sched_add_thread(thread_t *thread);

void sched_thread_continue(thread_t *thread);
void sched_thread_stop(thread_t *thread);
void sched_thread_exit(void);
void sched_thread_kill(thread_t *thread);
void sched_thread_sleep(uint32_t useconds);

thread_t *sched_current_thread(void);

// implemented by architecture-specific code
// TODO: document what functions are expected to be implemented by
//       architecture code
unsigned sched_num_cpus(void);
unsigned sched_current_cpu(void);

#endif
