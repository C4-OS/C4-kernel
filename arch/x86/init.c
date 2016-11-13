#include <c4/arch/segments.h>
#include <c4/arch/interrupts.h>
#include <c4/arch/ioports.h>
#include <c4/arch/pic.h>
#include <c4/arch/multiboot.h>
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
	sched_switch_thread( );
}

void test_thread_client( void *foo ){
	unsigned n = 0;
	debug_printf( "sup man\n" );

	while ( true ){
		message_t buf;

		//debug_printf( "sup man\n"j);
		message_recieve( &buf );

		debug_printf( "got a message from %u: %u, type: 0x%x\n",
		              buf.sender, buf.data[0], buf.type );

		if ( n % 4 == 0 ){
			//message_send( &buf, 3 );
		}
	}
}

void test_thread_meh( void *foo ){
	while ( true ){
		message_t buf;

		for ( unsigned k = 0; k < 20; k++ ){
			sched_thread_yield( );
		}

		message_recieve( &buf );

		debug_printf( ">>> buzz, %u\n", buf.data[0] );
	}
}

void test_thread_a( void *foo ){
	for (unsigned n = 0 ; n < 3; n++) {
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

void test_thread_d( void *foo ){
	debug_puts( "yo\n" );
	for ( ;; );
}

#define DO_SYSCALL(N, A, B, C, RET) \
	asm volatile ( " \
		mov %1, %%eax; \
		mov %2, %%edi; \
		mov %3, %%esi; \
		mov %4, %%edx; \
		int $0x60;     \
		mov %%eax, %0  \
	" : "=r"(RET) \
	  : "g"(N), "g"(A), "g"(B), "g"(C) \
	  : "eax", "edi", "esi", "edx" );

#include <c4/syscall.h>

void meh( void ){
	message_t msg;
	int a = 0;
	int ret = 0;

	while ( true ){
		a++;
		DO_SYSCALL( SYSCALL_RECIEVE, &msg, 0, 0, ret );
		DO_SYSCALL( SYSCALL_SEND,    &msg, 2, 0, ret );
	}
}

void sigma0_load( multiboot_module_t *module ){
	page_dir_t *new_pagedir = clone_page_dir( page_get_kernel_dir( ));

	set_page_dir( new_pagedir );

	unsigned func_size = module->end - module->start;
	void *sigma0_addr  = (void *)low_phys_to_virt(module->start);
	void *func         = map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );
	void *new_stack    = map_page( PAGE_READ | PAGE_WRITE, (void*)0xc0000000 );

	for ( uintptr_t foo = 0xbfff0000; foo < 0xbffff000; foo += PAGE_SIZE ){
		map_page( PAGE_READ | PAGE_WRITE, (void *)foo );
	}

	map_phys_page( PAGE_READ | PAGE_WRITE, (void *)0xb8000, (void *)0xb8000 );

	new_stack = (void *)((uintptr_t)new_stack + (0xf00));
	memcpy( func, sigma0_addr, func_size );

	thread_t *new_thread =
		thread_create( func, NULL, new_pagedir, new_stack, THREAD_FLAG_USER );

	set_page_dir( page_get_kernel_dir( ));

	sched_add_thread( new_thread );
}

void keyboard_handler( interrupt_frame_t *frame ){
	unsigned scancode = inb( 0x60 );
	bool key_up = !!(scancode & 0x80);
	scancode &= ~0x80;

	debug_printf( "ps2 keyboard: got scancode %u (%s)\n",
		scancode, key_up? "release" : "press" );

	message_t msg = {
		.type = 0xbeef,
		.data = { scancode, key_up },
	};

	message_try_send( &msg, 1 );
}

multiboot_module_t *sigma0_find_module( multiboot_header_t *header ){
	multiboot_module_t *ret = NULL;

	debug_printf( "multiboot header at %p\n", header );
	debug_printf( "    mod count: %u\n", header->mods_count );
	debug_printf( "    mod addr:  0x%x\n", low_phys_to_virt( header->mods_addr ));

	if ( header->mods_count > 0 ){
		ret = (void *)low_phys_to_virt( header->mods_addr );

		debug_printf( "    mod start: 0x%x\n", ret->start );
		debug_printf( "    mod end:   0x%x\n", ret->end );

		if ( ret->string ){
			char *temp = (char *)low_phys_to_virt( ret->string );
			debug_printf( "    mod strng: \"%s\"\n", temp );
		}
	}

	return ret;
}

void arch_init( multiboot_header_t *header ){
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
	region_init_global( (void *)(KERNEL_BASE + 0x400000) );
	debug_puts( "done\n" );

	debug_puts( "Initializing threading... " );
	init_threading( );
	debug_puts( "done\n" );

	debug_puts( "Initializing scheduler... " );
	init_scheduler( );
	debug_puts( "done\n" );

	multiboot_module_t *sigma0 = sigma0_find_module( header );

	if ( !sigma0 ){
		debug_printf( "Couldn't find a sigma0 binary, can't continue...\n" );
		return;
	}

	sigma0_load( sigma0 );
	sched_add_thread( thread_create_kthread( test_thread_client, NULL ));
	sched_add_thread( thread_create_kthread( test_thread_meh, NULL ));
	sched_add_thread( thread_create_kthread( test_thread_a, NULL ));
	sched_add_thread( thread_create_kthread( test_thread_client, NULL ));
	sigma0_load( sigma0 );

	register_interrupt( INTERRUPT_TIMER,    timer_handler );
	register_interrupt( INTERRUPT_KEYBOARD, keyboard_handler );

	asm volatile ( "sti" );

	for ( ;; );
}
