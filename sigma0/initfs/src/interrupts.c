#include <sigma0/sigma0.h>
#include <c4/arch/interrupts.h>
#include <stdint.h>
#include <stdbool.h>

static inline uint8_t c4_inbyte( unsigned port ){
	int ret;

	DO_SYSCALL( SYSCALL_IOPORT, SYSCALL_IO_INPUT, port, 0, 0, ret );

	return ret;
}

static inline void c4_outbyte( unsigned port, uint8_t value ){
	int ret;

	DO_SYSCALL( SYSCALL_IOPORT, SYSCALL_IO_OUTPUT, port, value, 0, ret );

	return;
}

int c4_msg_send( message_t *buffer, unsigned to ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_SEND, buffer, to, 0, 0, ret );

	return ret;
}

int c4_msg_recieve_async( message_t *buffer, unsigned flags ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_RECIEVE_ASYNC, buffer, flags, 0, 0, ret );

	return ret;
}

void _start( void *data ){
	uintptr_t display = (uintptr_t)data;

	message_t msg = {
		.type = MESSAGE_TYPE_INTERRUPT_SUBSCRIBE,
		.data = { INTERRUPT_KEYBOARD, },
	};

	c4_msg_send( &msg, 0 );

	while ( true ){
		c4_msg_recieve_async( &msg, MESSAGE_ASYNC_BLOCK );

		unsigned scancode = c4_inbyte( 0x60 );
		bool     key_up   = !!(scancode & 0x80);

		scancode &= ~0x80;

		msg.type = 0xbeef;
		msg.data[0] = scancode;
		msg.data[1] = key_up;

		c4_msg_send( &msg, display );
	}
}
