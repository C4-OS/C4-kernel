#include <c4/arch/segments.h>
#include <c4/arch/interrupts.h>
#include <c4/arch/ioports.h>
#include <c4/arch/pic.h>
#include <c4/paging.h>
#include <c4/debug.h>

#include <c4/klib/bitmap.h>
#include <c4/klib/string.h>
#include <c4/mm/region.h>
#include <c4/mm/slab.h>
#include <c4/common.h>

void arch_init( void ){
	slab_t   slab;

	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Initializing GDT... " );
	init_segment_descs( );
	debug_puts( "done\n" );

	debug_puts( "Initializing PIC..." );
	remap_pic_vectors_default( );
	debug_puts( "done\n" );

	debug_puts( "Initializing interrupts... " );
	init_interrupts( );
	debug_puts( "done\n" );

	debug_puts( "Initializing more paging structures... ");
	init_paging( );
	debug_puts( "done\n" );

	debug_puts( "Initializing kernel region... " );
	region_init_global( );
	debug_puts( "done\n" );

	/*
	slab_init_at( &slab, region_get_global(), sizeof(char[32]), NULL, NULL );

	debug_printf( "trying slab allocation... " );

	void *foo = slab_alloc( &slab );
	memset( foo, 0, sizeof(char[32]));
	debug_printf( "got %p, freeing\n", foo );
	slab_free( &slab, foo );
	*/

	asm volatile ( "sti" );
	asm volatile ( "int $1" );

	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );

	*(uint8_t *)(0xa0000000) = 123;
	debug_printf( "testing: %x\n", *(page_dir_t *)(0xfffff000));

	unmap_page( (void*)0xa0000000 );
	unmap_page( (void*)0xa0000000 );
	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );
	*(uint8_t *)(0xa0000000) = 123;

	debug_puts( "hello, world!\n" );

	for ( ;; );
}
