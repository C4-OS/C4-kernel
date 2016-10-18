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

#include <c4/thread.h>
#include <c4/scheduler.h>

void timer_handler( interrupt_frame_t *frame ){
	static unsigned n = 0;

	debug_printf( "timer! %u, ", n++ );
	sched_switch_thread( );
}

void test_thread_a( void *foo ){
	for (unsigned n = 0 ;; n++) {
		debug_printf( "foo! : +%u\n", n );
	}
}

void test_thread_b( void *foo ){
	for (unsigned n = 0 ;; n++) {
		debug_printf( "bar! : -%u\n", n );
	}
}

void test_thread_c( void *foo ){
	for (unsigned n = 0 ;; n++) {
		debug_printf( "baz! : -%u\n", n );
	}
}

void arch_init( void ){
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

	debug_puts( "Initializing threading... " );
	init_threading( );
	debug_puts( "done\n" );

	debug_puts( "Initializing scheduler... " );
	init_scheduler( );
	debug_puts( "done\n" );

	/*
	thread_list_t tlist;
	memset( &tlist, 0, sizeof( thread_list_t ));

	for ( unsigned i = 3; i; i-- ) {
		thread_t *foo = thread_create( test_thread, NULL );
		//debug_printf( "created thread at %p\n", foo );
		thread_list_insert( &tlist, foo );
	}

	thread_t *foo = thread_list_pop( &tlist );

	while ( foo ){
		debug_printf( "have thread %p (%u)\n", foo, sizeof( thread_t ));

		thread_destroy( foo );
		foo = thread_list_pop( &tlist );
	}
	*/

	sched_add_thread( thread_create( test_thread_a, NULL ));
	sched_add_thread( thread_create( test_thread_b, NULL ));
	sched_add_thread( thread_create( test_thread_c, NULL ));

	register_interrupt( INTERRUPT_TIMER, timer_handler );

	asm volatile ( "int $1" );

	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );

	*(uint8_t *)(0xa0000000) = 123;
	debug_printf( "testing: %x\n", *(page_dir_t *)(0xfffff000));

	unmap_page( (void*)0xa0000000 );
	unmap_page( (void*)0xa0000000 );
	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );
	*(uint8_t *)(0xa0000000) = 123;

	debug_puts( "hello, world!\n" );

	asm volatile ( "sti" );

	for ( ;; );
}
