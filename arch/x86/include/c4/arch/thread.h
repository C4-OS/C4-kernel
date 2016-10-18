#ifndef _C4_ARCH_THREAD_H
#define _C4_ARCH_THREAD_H 1
#include <stdint.h>

typedef struct thread thread_t;

typedef struct thread_registers {
	uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip;
} thread_regs_t;

#define THREAD_SAVE_STATE(T) { \
		asm volatile ( "mov %%esp, %0" : "=r"((T)->registers.esp) ); \
		asm volatile ( "mov %%ebp, %0" : "=r"((T)->registers.ebp) ); \
		asm volatile ( \
			"jmp 2f;" \
			"1: mov (%%esp), %0; ret;" \
			"2:" \
			"call 1b;" : "=r"((T)->registers.eip) \
		); \
	}

#define THREAD_RESTORE_STATE(T) { \
		asm volatile ( "mov %0, %%esp" :: "r"((T)->registers.esp) ); \
		asm volatile ( "mov %0, %%ebp" :: "r"((T)->registers.ebp) ); \
		asm volatile ( "mov %0, %%ecx" :: "r"((T)->registers.eip) ); \
		asm volatile ( "sti" ); \
		asm volatile ( "jmp *%ecx" ); \
	}

void thread_set_init_state( thread_t *thread,
                            void (*entry)(void *data),
                            void *data );

#endif
