#ifndef _C4_ARCH_THREAD_H
#define _C4_ARCH_THREAD_H 1
#include <stdint.h>

typedef struct thread thread_t;

typedef struct thread_registers {
	uint32_t ebp, esp, eip;
} thread_regs_t;

#endif
