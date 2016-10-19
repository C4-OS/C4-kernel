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
#include <c4/message.h>

void timer_handler( interrupt_frame_t *frame ){
	static unsigned n = 0;

	if ( (++n % 2) == 0 ){
		message_t msg = { n };
		//thread_t *foo = sched_get_thread_by_id( 2 );

		//debug_printf( "send message: %u \n", message_try_send( &msg, 2 ));
		message_try_send( &msg, 2 );
	}

	sched_switch_thread( );
}

void test_thread_client( void *foo ){
	unsigned n = 0;
	debug_printf( "sup man\n" );

	while ( true ){
		message_t buf;

		//debug_printf( "sup man\n"j);
		message_recieve( &buf );

		debug_printf( "got a message: %u,  %u\n", buf.data, n++ );

		if ( n % 4 == 0 ){
			message_send( &buf, 3 );
		}
	}
}

void test_thread_meh( void *foo ){
	unsigned n = 0;

	while ( true ){
		message_t buf;

		for ( unsigned k = 0; k < 50; k++ ){
			asm volatile ( "pusha;"
						   "call sched_switch_thread;"
						   "popa;" );
		}

		message_recieve( &buf );

		debug_printf( ">>> buzz, %u\n", buf.data );
	}
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

	sched_add_thread( thread_create( test_thread_client, NULL ));
	sched_add_thread( thread_create( test_thread_meh, NULL ));
	/*
	sched_add_thread( thread_create( test_thread_a, NULL ));
	sched_add_thread( thread_create( test_thread_b, NULL ));
	sched_add_thread( thread_create( test_thread_c, NULL ));
	*/

	register_interrupt( INTERRUPT_TIMER, timer_handler );

	asm volatile ( "sti" );

	for ( ;; );
}
