#include <c4/arch/segments.h>
#include <c4/arch/interrupts.h>
#include <c4/arch/paging.h>
#include <c4/debug.h>

void arch_init( void ){
	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Initializing gdt... " );
	init_segment_descs( );
	debug_puts( "done\n" );

	debug_puts( "Initializing interrupts... " );
	init_interrupts( );
	debug_puts( "done\n" );

	debug_puts( "Initializing more paging structures... ");
	init_paging( );
	debug_puts( "done\n" );

	//asm volatile ( "sti" );
	asm volatile ( "int $1" );
	asm volatile ( "int $3" );
	//asm volatile ( "int $5" );
	//asm volatile ( "int $15" );

	//volatile int a = 0;
	//asm volatile ("cli");

	//for ( volatile unsigned i = 0; i < 0xffffffff; i++ ){
		//debug_puts( "a" );
	//}
	//while ( 1 );
	*(uint8_t *)(0xa00000) = 123;

	debug_puts( "hello, world!\n" );
}
