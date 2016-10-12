#include <c4/arch/paging.h>
#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/klib/bitmap.h>
#include <c4/arch/earlyheap.h>
#include <c4/arch/interrupts.h>

#define NULL ((void *)0)

extern page_dir_t boot_page_dir;

static uint8_t *phys_page_bitmap;
unsigned avail_pages = 0;
unsigned first_free = 0;

// translate generic page flags to x86-specific ones
static inline unsigned page_flags( page_flags_t flags ){
	return (!!(flags & PAGE_WRITE) << 1)
	     | (!!(flags & PAGE_SUPERVISOR) << 2)
	     | PAGE_ARCH_PRESENT;
}

// TODO: parse multiboot structure to get available memory regions
static void *init_page_bitmap( void ){
	unsigned npages = 2048;
	phys_page_bitmap = kealloc( npages / 8 );

	// lowest 4MB is mapped in entry.s
	for ( unsigned i = 0; i < 1024; i++ ){
		bitmap_set( phys_page_bitmap, i );
	}

	avail_pages = npages - 1024;

	return phys_page_bitmap;
}

static void *alloc_phys_page( void ){
	if ( avail_pages == 0 ){
		return NULL;
	}

	unsigned i = first_free;
	unsigned offset;

	for ( i = 0; phys_page_bitmap[i] == 0xff; i++ );
	for ( offset = 0; phys_page_bitmap[i] & (1 << offset); offset++ );

	uintptr_t real = i*8 + offset;

	bitmap_set( phys_page_bitmap, real );

	first_free = i;
	avail_pages--;

	return (void *)(real * PAGE_SIZE);
}

void page_fault_handler( interrupt_frame_t *frame ){
	debug_printf( "=== page fault! ===\n" );
	debug_printf( "=== error code: 0b%b ===\n", frame->error_num );

	interrupt_print_frame( frame );

	for ( ;; );
}

void init_paging( void ){
	register_interrupt( INTERRUPT_PAGE_FAULT, page_fault_handler );

	init_page_bitmap( );
	debug_printf( " (%p)\n", &boot_page_dir );

	for ( unsigned i = 0; i < 1024; i++ ){
		debug_printf( "allocated phys page at %p, avail: %u \n",
			alloc_phys_page( ), avail_pages );
	}
}

void *map_page( page_dir_t *dir, unsigned permissions, void *vaddress ){
	return (void *)0;
}

void *map_phys_page( page_dir_t *dir, unsigned perm, void *vaddr, void *raddr );
void unmap_page( page_dir_t *dir, void *vaddress );
