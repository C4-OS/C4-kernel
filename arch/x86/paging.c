#include <c4/arch/paging.h>
#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/klib/bitmap.h>
#include <c4/arch/earlyheap.h>
#include <c4/arch/interrupts.h>

#define NULL ((void *)0)

extern page_dir_t boot_page_dir;
static page_dir_t *kernel_page_dir;

static bitmap_ent_t *phys_page_bitmap;
unsigned avail_pages = 0;
unsigned first_free = 0;

// translate generic page flags to x86-specific ones
static inline unsigned page_flags( page_flags_t flags ){
	return (!!(flags & PAGE_WRITE) << 1)
	     | (!!(flags & PAGE_SUPERVISOR) << 2)
	     | PAGE_ARCH_PRESENT;
}

// converts a page in the 4MB of initial kernel space to its
// corresponding physical page
static inline uintptr_t low_virt_to_phys( uintptr_t addr ){
	return addr - 0xc0000000;
}

static inline void *add_page_flags( void *addr, page_flags_t flags ){
	return (void *)((uintptr_t)addr | page_flags(flags));
}

static inline void flush_tlb( void ){
	asm volatile (
		"mov %cr3, %eax;"
		"mov %eax, %cr3;"
	);
}

static inline void invalidate_page( void *paddr ){
	asm volatile ( "invlpg %0" :: "m"(paddr) );
}

// TODO: parse multiboot structure to get available memory regions
//       for now assume the machine has 8MB of linear physical memory
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

// TODO: maybe look into more efficient method for page allocation,
//       using a bitmap is O(n) in the worst case. With heavy physical
//       fragmentation it might become a performance problem.
static void *alloc_phys_page( void ){
	if ( avail_pages == 0 ){
		return NULL;
	}

	unsigned i = first_free;
	unsigned offset;

	for ( i = 0; phys_page_bitmap[i] == BITMAP_ENT_FULL; i++ );
	for ( offset = 0; phys_page_bitmap[i] & (1 << offset); offset++ );

	uintptr_t real = i * BITMAP_BPS + offset;

	bitmap_set( phys_page_bitmap, real );

	first_free = i;
	avail_pages--;

	return (void *)(real * PAGE_SIZE);
}

static void free_phys_page( void *addr ){
	uintptr_t temp = (uintptr_t)addr / PAGE_SIZE;
	unsigned pos = temp / BITMAP_BPS;

	bitmap_unset( phys_page_bitmap, temp );
	avail_pages++;

	if ( pos < first_free ){
		first_free = pos;
	}
}

void page_fault_handler( interrupt_frame_t *frame ){
	uint32_t cr_2;

	asm volatile ( "mov %%cr2, %0" : "=r"(cr_2));

	debug_printf( "=== page fault! ===\n" );
	debug_printf( "=== error code: 0b%b ===\n", frame->error_num );
	debug_printf( "=== fault address: %p\n", cr_2 );

	interrupt_print_frame( frame );

	for ( ;; );
}

void init_paging( void ){
	kernel_page_dir = &boot_page_dir;

	// set up recursive mapping
	kernel_page_dir[1023] =
		low_virt_to_phys((uintptr_t)kernel_page_dir)
		| PAGE_ARCH_PRESENT
		| PAGE_ARCH_WRITABLE
		| PAGE_ARCH_SUPERVISOR
		;

	register_interrupt( INTERRUPT_PAGE_FAULT, page_fault_handler );

	init_page_bitmap( );
	debug_printf( " (%p)\n", kernel_page_dir );
}

void *map_page( unsigned perms, void *vaddress ){
	unsigned dirent   = page_dir_entry( vaddress );
	unsigned tableent = page_table_entry( vaddress );

	page_dir_t *dir     = current_page_dir( );
	page_table_t *table = (void *)(0xffc00000 | (dirent << 12));

	if ( dir[dirent] ){
		debug_printf( "have existing page directory entry for %p at %p\n",
		  vaddress, dir[dirent] );

	} else {
		dir[dirent] =
			(page_table_t)add_page_flags(
				alloc_phys_page( ),
				PAGE_ARCH_PRESENT | PAGE_ARCH_WRITABLE );

		debug_printf( "do not have a directory entry for %p, mapped %p\n",
			  vaddress, dir[dirent] );
	}

	table[tableent] = (page_table_t)add_page_flags( alloc_phys_page(), perms );
	debug_printf( "mapped %p for table entry\n", table[tableent] );

	return (void *)vaddress;
}

void *map_phys_page( unsigned perm, void *vaddr, void *raddr );

void unmap_page( void *vaddress ){
	unsigned dirent   = page_dir_entry( vaddress );
	unsigned tableent = page_table_entry( vaddress );

	page_dir_t *dir     = current_page_dir( );
	page_table_t *table = (void *)(0xffc00000 | (dirent << 12));

	if ( dir[dirent] ){
		if ( table[tableent] ){
			void *paddr = (void *)table[tableent];
			debug_printf( "vaddress %p is mapped, unmapping...\n", vaddress );

			table[tableent] = 0;
			invalidate_page( paddr );
			free_phys_page( paddr );
		}
	}
}

page_dir_t *current_page_dir( void ){
	// TODO: read cr3
	return (page_dir_t *)0xfffff000;
}