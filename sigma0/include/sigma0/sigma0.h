#ifndef _C4_SIGMA0_H
#define _C4_SIGMA0_H 1
#include <c4/syscall.h>
#include <c4/message.h>
#include <stdbool.h>

#define NULL ((void *)0)

#define DO_SYSCALL(N, A, B, C, D, RET) \
	asm volatile ( " \
		mov %1, %%eax; \
		mov %2, %%edi; \
		mov %3, %%esi; \
		mov %4, %%edx; \
		mov %5, %%ebx; \
		int $0x60;     \
		mov %%eax, %0  \
	" : "=r"(RET) \
	  : "g"(N), "g"(A), "g"(B), "g"(C), "g"(D) \
	  : "eax", "edi", "esi", "edx", "ebx" );

void server( void * );

void display_thread( void *unused );
int c4_msg_send( message_t *buffer, unsigned target );
int c4_msg_recieve( message_t *buffer, unsigned whom );
int c4_create_thread( void (*entry)(void *), void *stack, unsigned flags );
int c4_continue_thread( unsigned thread );

int c4_mem_map_to( unsigned thread_id, void *from, void *to,
                   unsigned size, unsigned permissions );
int c4_mem_grant_to( unsigned thread_id, void *from, void *to,
                     unsigned size, unsigned permissions );

void *c4_request_physical( uintptr_t virt,
                           uintptr_t physical,
                           unsigned size,
                           unsigned permissions );

char decode_scancode( unsigned long code );

#endif
