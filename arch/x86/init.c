#include <c4/arch/segments.h>
#include <c4/arch/interrupts.h>
#include <c4/debug.h>

void arch_init( void ){
	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Initializing gdt... " );
	init_segment_descs( );
	debug_puts( "done\n" );

	debug_puts( "Initializing interrupts... " );
	init_interrupts( );
	debug_puts( "done\n" );

	asm volatile ( "sti" );
	asm volatile ( "int $0" );
	asm volatile ( "int $3" );

	debug_puts( "hello, world!\n" );
}
