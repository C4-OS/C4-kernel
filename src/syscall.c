#include <c4/syscall.h>
#include <c4/debug.h>
#include <c4/message.h>

typedef int (*syscall_func_t)( uintptr_t a, uintptr_t b, uintptr_t c );

static int syscall_exit( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_create_thread( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_send( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_send_async( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_recieve( uintptr_t a, uintptr_t b, uintptr_t c );

static const syscall_func_t syscall_table[SYSCALL_MAX] = {
	syscall_exit,
	syscall_create_thread,
	syscall_send,
	syscall_send_async,
	syscall_recieve,
};

int syscall_dispatch( unsigned num, uintptr_t a, uintptr_t b, uintptr_t c ){
	if ( num >= SYSCALL_MAX ){
		return -1;
	}

	return syscall_table[num](a, b, c);
}

static int syscall_exit( uintptr_t a, uintptr_t b, uintptr_t c ){
	debug_printf( "got exit with %u, %u, and %u\n", a, b, c );

	return 0;
}

static int syscall_create_thread( uintptr_t a, uintptr_t b, uintptr_t c ){
	return 0;
}

static int syscall_send( uintptr_t buffer, uintptr_t target, uintptr_t c ){
	message_t *msg = (message_t *)buffer;
	debug_printf( "trying to send message %p to %u\n", msg, target );

	message_send( msg, target );

	return 0;
}

static int syscall_send_async( uintptr_t a, uintptr_t b, uintptr_t c ){
	return 0;
}

static int syscall_recieve( uintptr_t buffer, uintptr_t b, uintptr_t c ){
	message_t *msg = (message_t *)buffer;
	debug_printf( "trying to recieve message at %p\n", msg );

	message_recieve( msg );

	return 0;
}

