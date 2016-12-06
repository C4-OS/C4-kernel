#include <sigma0/sigma0.h>
#include <stdint.h>
#include <stdbool.h>

void main( void *data ){
	uintptr_t display = (uintptr_t)data;
	message_t msg = {
		.type = 0xcafe,
		.data = { 'A', },
	};

	while ( true ){
		int ret;
		for ( volatile unsigned k = 0; k < 1000000; k++ );
		DO_SYSCALL( SYSCALL_SEND, &msg, display, 0, 0, ret );
	}
}
