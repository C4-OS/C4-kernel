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
#include <c4/bootinfo.h>

void timer_handler( interrupt_frame_t *frame ){
	sched_switch_thread( );
}

static inline bool is_valid_vbe( vbe_mode_t *mode ){
	return mode->physbase != 0
	    && mode->bpp      != 0
	    && mode->bpp      != 0xff;
}

// initialize a bootinfo_t structure from a multiboot header
static void bootinfo_init( bootinfo_t *info, multiboot_header_t *header ){
	char *cmd = "c4";

	info->magic = BOOTINFO_MAGIC;
	info->num_pages = 0;

	info->version.major = 0;
	info->version.minor = 0;
	info->version.patch = 0;

	if ( FLAG( header, MULTIBOOT_FLAG_CMDLINE )){
		cmd = (char *)low_phys_to_virt( header->cmdline );
		debug_printf( "have command line, " );

	} else {
		debug_printf( "no command line, " );
	}

	strncpy( info->cmdline, cmd, sizeof( info->cmdline ));

	info->framebuffer.exists = false;

	if ( FLAG( header, MULTIBOOT_FLAG_VBE )){
		vbe_mode_t *mode = (void*)low_phys_to_virt( header->vbe_mode_info );

		if ( is_valid_vbe( mode )){
			info->framebuffer.exists = true;
			info->framebuffer.width  = mode->x_res;
			info->framebuffer.height = mode->y_res;
			info->framebuffer.addr   = mode->physbase;
			info->framebuffer.bpp    = mode->bpp;

			debug_printf( "framebuffer %ux%ux%u @ %p, ",
				info->framebuffer.width,
				info->framebuffer.height,
				info->framebuffer.bpp,
				info->framebuffer.addr );

		} else {
			debug_printf(
				"bootloader says we have graphics, but info is invalid, " );
		}

	} else {
		debug_printf( "no framebuffer, " );
	}
}

void sigma0_load( multiboot_module_t *module, bootinfo_t *bootinfo ){
	addr_space_t *new_space = addr_space_clone( addr_space_kernel( ));

	addr_space_set( new_space );

	// XXX: 0x200 is maximum bss size, since the bss won't be part of the flat
	//      binary that is produced by objcopy for sigma0.
	//      It might need to be increased in the future, maybe provide a config
	//      option, once that's implemented?
	unsigned func_size = module->end - module->start + 0x200;
	void *sigma0_addr  = (void *)low_phys_to_virt(module->start);
	addr_entry_t ent;

	uintptr_t code_start = 0xc0000000;
	uintptr_t code_end   = code_start + func_size +
	                       (PAGE_SIZE - (func_size % PAGE_SIZE));

	uintptr_t data_start = 0xd0000000;
	uintptr_t data_end   = 0xd0800000;

	void *func      = (void *)code_start;
	void *new_stack = (void *)(data_start + 0xff8);

	ent = (addr_entry_t){
		.virtual     = (uintptr_t)BOOTINFO_ADDR,
		.physical    = 0x800000,
		.size        = 1,
		.permissions = PAGE_READ,
	};

	addr_space_insert_map( new_space, &ent );

	// bootinfo_addr defined in bootinfo.h
	memcpy( BOOTINFO_ADDR, bootinfo, sizeof( bootinfo_t ));

	ent = (addr_entry_t){
		.virtual     = code_start,
		.physical    = 0x810000,
		.size        = (code_end - code_start) / PAGE_SIZE,
		.permissions = PAGE_READ | PAGE_WRITE,
	};

	addr_space_insert_map( new_space, &ent );

	ent = (addr_entry_t){
		.virtual     = data_start,
		.physical    = 0x850000,
		.size        = (data_end - data_start) / PAGE_SIZE,
		.permissions = PAGE_READ | PAGE_WRITE,
	};

	addr_space_insert_map( new_space, &ent );
	addr_space_map_self( new_space, ADDR_MAP_ADDR );
	debug_printf( "asdf: 0x%x\n", code_end );

	memcpy( func, sigma0_addr, func_size );

	thread_t *new_thread =
		thread_create( func, new_space, new_stack, THREAD_FLAG_USER );

	set_page_dir( page_get_kernel_dir( ));

	sched_add_thread( new_thread );
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

void test_thread_client( void ){
	unsigned n = 0;
	debug_printf( "sup man\n" );

	while ( true ){
		asm volatile ("hlt");
		/*
		message_t buf;

		debug_printf( "sup man\n"j);
		//message_recieve( &buf, 0 );
		message_recieve( queue, &buf );

		debug_printf( "got a message from %u: %u, type: 0x%x\n",
		              buf.sender, buf.data[0], buf.type );

		if ( n % 4 == 0 ){
			//message_send( &buf, 3 );
		}
		*/
	}
}

#include <c4/mm/addrspace.h>

void arch_init( multiboot_header_t *header ){
	static bootinfo_t bootinfo;

	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Storing boot info... " );
	bootinfo_init( &bootinfo, header );
	debug_puts( "done\n" );

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

	debug_puts( "Initializing capability space structures... " );
	//cap_space_init( );
	debug_puts( "done\n" );

	debug_puts( "Initializing address space structures... " );
	addr_space_init( );
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

	sigma0_load( sigma0, &bootinfo );

	sched_add_thread( thread_create_kthread( test_thread_client ));
	register_interrupt( INTERRUPT_TIMER, timer_handler );

	asm volatile ( "sti" );
	for ( ;; );
}
