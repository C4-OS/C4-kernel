#include <c4/arch/mp/pit.h>
#include <stdbool.h>

static inline void outb( uint16_t port, uint8_t value ){
	asm volatile( "outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb( uint16_t port ){
	uint8_t value = 0;
	asm volatile( "inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

void pit_wait_poll( uint16_t value ){
	outb( PIT_COMMAND, PIT_CMD_HILOBYTE );
	outb( PIT_CHAN0,   value & 0xff );
	outb( PIT_CHAN0,   value >> 8 );

	uint8_t status = 0;

	while ((status & PIT_STATUS_STATE) == false ){
		outb( PIT_COMMAND, PIT_CMD_READ_BACK | PIT_READ_LATCH_COUNT | PIT_READ_CHAN0 );
		status = inb( PIT_CHAN0 );
	}
}
