#ifndef _C4_SYSCALL_H
#define _C4_SYSCALL_H 1
#include <stdint.h>

enum {
	// thread syscalls
	SYSCALL_THREAD_EXIT,
	SYSCALL_THREAD_CREATE,
	SYSCALL_THREAD_SET_ADDRSPACE,
	SYSCALL_THREAD_SET_CAPSPACE,
	SYSCALL_THREAD_SET_STACK,
	SYSCALL_THREAD_SET_IP,
	SYSCALL_THREAD_SET_PRIORITY,
	SYSCALL_THREAD_SET_PAGER,
	SYSCALL_THREAD_CONTINUE,
	SYSCALL_THREAD_STOP,

	// syncronous ipc syscalls
	SYSCALL_SYNC_CREATE,
	SYSCALL_SYNC_SEND,
	SYSCALL_SYNC_RECIEVE,

	// asyncronous ipc syscalls
	SYSCALL_ASYNC_CREATE,
	SYSCALL_ASYNC_SEND,
	SYSCALL_ASYNC_RECIEVE,

	// address space syscalls
	SYSCALL_ADDRSPACE_CREATE,
	SYSCALL_ADDRSPACE_MAP,
	SYSCALL_ADDRSPACE_UNMAP,

	// physical frame syscalls
	SYSCALL_PHYS_FRAME_CREATE,
	SYSCALL_PHYS_FRAME_SPLIT,
	SYSCALL_PHYS_FRAME_JOIN,

	// capapbility space management syscalls
	SYSCALL_CSPACE_CREATE,
	SYSCALL_CSPACE_CAP_MOVE,
	SYSCALL_CSPACE_CAP_COPY,
	SYSCALL_CSPACE_CAP_REMOVE,
	SYSCALL_CSPACE_CAP_RESTRICT,
	SYSCALL_CSPACE_CAP_SHARE,
	SYSCALL_CSPACE_CAP_GRANT,

	// other syscalls
	SYSCALL_INTERRUPT_SUBSCRIBE,
	SYSCALL_INTERRUPT_UNSUBSCRIBE,
	SYSCALL_IOPORT,
	SYSCALL_INFO,
	SYSCALL_DEBUG_PUTCHAR,
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

enum {
	SYSCALL_INFO_GET_ID,
	//SYSCALL_INFO_GET_PAGER,
};

// called from the appropriate syscall interface for the platform,
// see arch/<platform>/syscall.c
int syscall_dispatch( unsigned num,
                      uintptr_t a, uintptr_t b,
                      uintptr_t c, uintptr_t d );

#endif
