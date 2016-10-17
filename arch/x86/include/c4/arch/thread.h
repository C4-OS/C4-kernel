#ifndef _C4_ARCH_THREAD_H
#define _C4_ARCH_THREAD_H 1
#include <stdint.h>

typedef struct thread thread_t;

typedef struct thread_registers {
	uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip;
} thread_regs_t;

void thread_store_registers( thread_regs_t *buf );
void thread_set_init_state( thread_t *thread, void (*entry)(void *data) );

#endif
