#ifndef _C4_ARCH_THREAD_H
#define _C4_ARCH_THREAD_H 1
#include <stdint.h>

typedef struct thread thread_t;

typedef struct thread_registers {
	uint32_t ebp, esp, eip;
	// this is used to determine whether this thread has already been
	// run and is in a user context
	uint32_t do_user_switch;
} thread_regs_t;

#endif
