//Generic timer interface
#ifndef _C4_TIMER_H
#define _C4_TIMER_H 1
#include <c4/kobject.h>
#include <stdint.h>

typedef struct user_timer {
	kobject_t object;
	uint64_t  timestamp;
} timer_user_t;

// ARCH: implemented in architecture-specific code
void timers_init(void);
uint64_t timer_uptime(void);
uint64_t timer_get_timestamp(void);
uint64_t timer_get_timestamp_ns(void);

// generic userland functions
uint64_t timer_elapsed(timer_user_t *timestamp);
void timer_reset(timer_user_t *timer);

#endif
