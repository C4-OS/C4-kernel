#ifndef _C4_SYSCALL_H
#define _C4_SYSCALL_H 1
#include <stdint.h>

enum {
	SYSCALL_EXIT,
	SYSCALL_CREATE_THREAD,
	SYSCALL_SEND,
	SYSCALL_RECIEVE,
	SYSCALL_SEND_ASYNC,
	SYSCALL_RECIEVE_ASYNC,
	SYSCALL_IOPORT,
	SYSCALL_MAX,
};

// XXX: architecture-specific workaround, will need to be removed in the future
enum {
	IO_PORT_IN_BYTE,
	IO_PORT_IN_WORD,
	IO_PORT_IN_DWORD,
	IO_PORT_OUT_BYTE,
	IO_PORT_OUT_WORD,
	IO_PORT_OUT_DWORD,
};

// called from the appropriate syscall interface for the platform,
// see arch/<platform>/syscall.c
int syscall_dispatch( unsigned num,
                      uintptr_t a, uintptr_t b,
                      uintptr_t c, uintptr_t d );

#endif
