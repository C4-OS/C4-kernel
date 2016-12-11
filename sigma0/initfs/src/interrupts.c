#include <sigma0/sigma0.h>
#include <c4/arch/interrupts.h>
#include <stdint.h>
#include <stdbool.h>

void _start( void *data ){
	uintptr_t display = (uintptr_t)data;
	int ret;

	volatile message_t msg;

	while ( true ){
		unsigned from = MESSAGE_INTERRUPT_MASK | INTERRUPT_KEYBOARD;

		DO_SYSCALL( SYSCALL_RECIEVE, &msg, from, 0, 0, ret );
	}
}
