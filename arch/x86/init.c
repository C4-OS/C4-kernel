#include <c4/arch/segments.h>
#include <c4/debug.h>

void arch_init( void ){
	debug_puts( "Initializing gdt... ");
	init_segment_descs( );
	debug_puts( "done\n\n" );

	debug_puts( "hello, world!\n" );
}
