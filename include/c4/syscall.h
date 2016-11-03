#ifndef _C4_SYSCALL_H
#define _C4_SYSCALL_H 1
#include <stdint.h>

enum {
	SYSCALL_EXIT,
	SYSCALL_CREATE_THREAD,
	SYSCALL_SEND,
	SYSCALL_SEND_ASYNC,
	SYSCALL_RECIEVE,
	SYSCALL_MAX,
};

// called from the appropriate syscall interface for the platform,
// see arch/<platform>/syscall.c
int syscall_dispatch( unsigned num, uintptr_t a, uintptr_t b, uintptr_t c );

#endif
