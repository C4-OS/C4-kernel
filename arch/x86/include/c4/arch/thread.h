#ifndef _C4_ARCH_THREAD_H
#define _C4_ARCH_THREAD_H 1
#include <stdint.h>

typedef struct thread thread_t;

typedef struct thread_registers {
	uint32_t ebp, esp, eip;
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
        asm volatile ( "mov %0, %%esp;" \
                       "mov %1, %%ebp;" \
                       "mov %2, %%eax;"  \
                       "sti;" \
                       "jmp *%%eax;" \
            :: "g"((T)->registers.esp), \
               "g"((T)->registers.ebp), \
               "g"((T)->registers.eip)  \
            : "eax" \
        ); \
    }

void thread_set_init_state( thread_t *thread,
                            void (*entry)(void *data),
                            void *data );

#endif
