#ifndef _C4_SIGMA0_H
#define _C4_SIGMA0_H 1
#include <c4/syscall.h>
#include <c4/message.h>
#include <stdbool.h>

#define NULL ((void *)0)

#define DO_SYSCALL(N, A, B, C, RET) \
	asm volatile ( " \
		mov %1, %%eax; \
		mov %2, %%edi; \
		mov %3, %%esi; \
		mov %4, %%edx; \
		int $0x60;     \
		mov %%eax, %0  \
	" : "=r"(RET) \
	  : "g"(N), "g"(A), "g"(B), "g"(C) \
	  : "eax", "edi", "esi", "edx" );

void main( void );
void server( void * );

void display_thread( void *unused );
int c4_msg_send( message_t *buffer, unsigned target );
int c4_msg_recieve( message_t *buffer, unsigned whom );

#endif
