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

#include <c4/arch/earlyheap.h>

typedef struct phys_memmap {
	struct phys_memmap *next;

	uintptr_t addr;
	size_t    length;
} phys_memmap_t;

static void dump_raw_memmaps( uintptr_t map, size_t length ){
	for ( size_t offset = 0; offset < length; ){
		multiboot_mem_map_t *foo = (void *)(map + offset);

		uintptr_t a = foo->addr_high;
		uintptr_t b = a + foo->len_high;

		char *s = (foo->type == MULTIBOOT_MEM_AVAILABLE)
			? "available"
			: "reserved";

		debug_printf( "%p (%u)> %p -> %p (%u: %s)\n",
					  foo, foo->size, a, b, foo->type, s );

		offset += foo->size + sizeof(foo->size);
	}
}

static void dump_cooked_memmaps( phys_memmap_t *maps ){
	for ( phys_memmap_t *map = maps; map; map = map->next ){
		debug_printf( "map %p > %p, %u bytes\n",
					  map->addr, map->addr + map->length,
					  map->length );
	}
}

static phys_memmap_t *memmaps_init( multiboot_header_t *mboot,
                                    phys_memmap_t **kmemptr )
{
	phys_memmap_t *ret = NULL;

	if ( !FLAG( mboot, MULTIBOOT_FLAG_MMAP )){
		debug_printf( "don't have memory map!\n" );
		return NULL;
	}

	uintptr_t map = low_phys_to_virt( mboot->mmap_addr );

	debug_printf( "have memory map: %p (%u bytes)\n",
				  map, mboot->mmap_length );

	dump_raw_memmaps( map, mboot->mmap_length );

	for ( size_t offset = 0; offset < mboot->mmap_length; ){
		multiboot_mem_map_t *foo = (void *)(map + offset);
		offset += foo->size + sizeof(foo->size);

		// Only register memory above the 1MB mark, since grub usually
		// puts all the boot info in low memory. Not that it's used after
		// boot anyway, but it's a small amount of memory so might as
		// well keep it around just in case.
		if ( foo->type == MULTIBOOT_MEM_AVAILABLE && foo->addr_high >= 0x100000 ){
			phys_memmap_t *nmap = kealloc( sizeof( phys_memmap_t ));
			KASSERT( nmap != NULL );

			nmap->next       = ret;
			nmap->addr       = foo->addr_high;
			nmap->length     = foo->len_high;
			ret = nmap;

			// TODO: configurable kernel memory size,
			//       this reserves 8MB from the region starting at 1MB,
			//       which is assumed to exist
			// TODO: better code here once new paging stuff works
			if ( nmap->addr == 0x100000 ){
				KASSERT( nmap->length > 0x800000 );
				phys_memmap_t *kmap = kealloc( sizeof( phys_memmap_t ));
				KASSERT( kmap != NULL );

				//kmap->addr        = nmap->addr;
				// XXX: start at 4MB, all mapped for the kernel at boot
				kmap->addr        = 0x400000;
				kmap->length      = 0x800000;
				kmap->next        = NULL;
				*kmemptr = kmap;

				nmap->addr       += 0x800000;
				nmap->length     -= 0x800000;
			}
		}
	}

	return ret;
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

	void *func      = (void *)code_start;
	void *new_stack = (void *)(data_start + 0xff8);

	// TODO: cut these from a block of memory found in memmaps_init()
	phys_frame_t *info = phys_frame_create( 0x800000, 1, 0 );
	phys_frame_t *code = phys_frame_create( 0x810000, 64, 0 );
	phys_frame_t *data = phys_frame_create( 0x850000, 64, 0 );

	addr_space_make_ent( &ent, (uintptr_t)BOOTINFO_ADDR, PAGE_READ, info );
	addr_space_insert_map( new_space, &ent );

	// bootinfo_addr defined in bootinfo.h
	memcpy( BOOTINFO_ADDR, bootinfo, sizeof( bootinfo_t ));

	addr_space_make_ent( &ent, code_start, PAGE_READ | PAGE_WRITE, code );
	addr_space_insert_map( new_space, &ent );

	addr_space_make_ent( &ent, data_start, PAGE_READ | PAGE_WRITE, data );
	addr_space_insert_map( new_space, &ent );
	debug_printf( "asdf: 0x%x\n", code_end );

	memcpy( func, sigma0_addr, func_size );

	thread_t *new_thread =
		thread_create( func, new_space, new_stack, THREAD_FLAG_USER );
	new_thread->cap_space = cap_space_create();

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
	//unsigned n = 0;
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

static void init_memory( phys_memmap_t *mem ){
	static region_t region;
	size_t npages = mem->length / PAGE_SIZE;
	bitmap_ent_t *map = kealloc( npages / 8 );

	debug_puts( "Initializing physical page map... ");
	region_init_at( &region, (void *)mem->addr,
	                map, npages,
	                PAGE_READ | PAGE_WRITE | PAGE_SUPERVISOR,
	                REGION_MODE_PHYSICAL );
	debug_puts( "done\n" );

	debug_puts( "Initializing more paging structures... ");
	init_paging( &region );
	debug_puts( "done\n" );

	debug_puts( "Initializing kernel region... " );
	region_init_global( (void *)(KERNEL_BASE + 0x400000) );
	debug_puts( "done\n" );
}

void arch_init( multiboot_header_t *header ){
	static bootinfo_t bootinfo;
	phys_memmap_t *kernel_mem = NULL;
	phys_memmap_t *memmaps = NULL;

	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Storing boot info... " );
	bootinfo_init( &bootinfo, header );
	debug_puts( "done\n" );
	memmaps = memmaps_init( header, &kernel_mem );
	dump_cooked_memmaps( memmaps );

	debug_puts( "Initializing GDT... " );
	init_segment_descs( );
	debug_puts( "done\n" );

	debug_puts( "Initializing PIC..." );
	remap_pic_vectors_default( );
	debug_puts( "done\n" );

	debug_puts( "Initializing interrupts... " );
	init_interrupts( );
	debug_puts( "done\n" );

	init_memory( kernel_mem );

	debug_puts( "Initializing capability space structures... " );
	cap_space_init( );
	debug_puts( "done\n" );

	debug_puts( "Initializing address space structures... " );
	phys_frame_init( );
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
