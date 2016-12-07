#include <sigma0/sigma0.h>
#include <stdint.h>
#include <stdbool.h>

void _start( void *data ){
	int ret;
	uintptr_t display = (uintptr_t)data;

	message_t msg = {
		.type = MESSAGE_TYPE_DEBUG_PRINT,
		.data = { 'A', },
	};

	// XXX: kernel hangs if this recieve call isn't made before the send call,
	//      will need to figure out what is causing that
	DO_SYSCALL( SYSCALL_RECIEVE, &msg, 0, 0, 0, ret );

	msg.data[0] = 'A';

	while ( true ){
		for ( volatile unsigned k = 0; k < 100000; k++ );
		DO_SYSCALL( SYSCALL_SEND, &msg, display, 0, 0, ret );
	}
}
