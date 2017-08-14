#include <c4/arch/paging.h>
#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/klib/bitmap.h>
#include <c4/arch/earlyheap.h>
#include <c4/arch/interrupts.h>
#include <c4/common.h>
#include <c4/mm/region.h>

#define NULL ((void *)0)

extern page_dir_t boot_page_dir;
static page_dir_t *kernel_page_dir;
region_t *physical_region;

// translate generic page flags to x86-specific ones
static inline unsigned page_flags( page_flags_t flags ){
	return ( !!(flags & PAGE_WRITE) << 1)
	     | (!!!(flags & PAGE_SUPERVISOR) << 2)
	     |  PAGE_ARCH_PRESENT;
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

// TODO: maybe look into more efficient method for page allocation,
//       using a bitmap is O(n) in the worst case. With heavy physical
//       fragmentation it might become a performance problem.
static inline void *alloc_phys_page( void ){
	return region_alloc( physical_region );
}

static inline void free_phys_page( void *addr ){
	region_free( physical_region, addr );
}

static page_table_t *page_current_table_entry( unsigned entry ){
	return (void *)(0xffc00000 | (entry << 12));
}

void *page_phys_addr( void *vaddress ){
	unsigned dirent   = page_dir_entry( vaddress );
	unsigned tableent = page_table_entry( vaddress );

	page_dir_t   *dir   = current_page_dir( );
	page_table_t *table = page_current_table_entry( dirent );
	uintptr_t    ret    = 0;

	if ( dir[dirent] ){
		if ( dir[dirent] & PAGE_ARCH_4MB_ENTRY ){
			//return (void *)(dir[dirent] | (tableent << 12));
			ret = dir[dirent] | (tableent << 12);

		} else if ( table[tableent] ){
			ret = table[tableent];
			//return (void *)(table[tableent] & ~PAGE_ARCH_ALL_FLAGS);
		}
	}

	if ( !ret ){
		debug_printf( "warning: have vaddress %p without phys. page\n", vaddress );
		for ( ;; );
	}

	return (void *)(ret & ~PAGE_ARCH_ACCESSED);
}

void page_fault_handler( interrupt_frame_t *frame ){
	unsigned err = frame->error_num;
	uint32_t cr_2;

	asm volatile ( "mov %%cr2, %0" : "=r"(cr_2));

	// if it's a page fault from usermode, handle that with generic paging
	// code and continue
	//
	// TODO: change 'supervisor' to 'user' in kernel
	if ( err & PAGE_ARCH_SUPERVISOR ){
		unsigned perms = (err & PAGE_ARCH_WRITABLE)? PAGE_WRITE : PAGE_READ;

		// do page fault messaging stuff
		page_fault_message( cr_2, frame->eip, perms );

	// otherwise panic
	} else {
		// TODO: panic() function
		debug_printf( "=== page fault! ===\n" );
		debug_printf( "=== fault address: %p\n", cr_2 );
		debug_printf( "=== error code: 0b%b ===\n", frame->error_num );
		debug_printf( "=== (%s, %s, %s) ===\n",
					  (err & PAGE_ARCH_PRESENT)?    "present"   : "not present",
					  (err & PAGE_ARCH_SUPERVISOR)? "user mode" : "supervisor",
					  (err & PAGE_ARCH_WRITABLE)?   "write"     : "read"
					);

		interrupt_print_frame( frame );
		for ( ;; );
	}
}

void init_paging( region_t *phys_map ){
	kernel_page_dir = &boot_page_dir;

	// set up recursive mapping
	kernel_page_dir[1023] =
		low_virt_to_phys((uintptr_t)kernel_page_dir)
		| PAGE_ARCH_PRESENT
		| PAGE_ARCH_WRITABLE
		//| PAGE_ARCH_SUPERVISOR
		;

	physical_region = phys_map;

	register_interrupt( INTERRUPT_PAGE_FAULT, page_fault_handler );
	debug_printf( " (%p)\n", kernel_page_dir );
}

void *map_page( unsigned perms, void *vaddr ){
	void *raddr = alloc_phys_page( );

	return map_phys_page( perms, vaddr, raddr );
}

void *map_phys_page( unsigned perms, void *vaddr, void *raddr ){
	unsigned dirent   = page_dir_entry( vaddr );
	unsigned tableent = page_table_entry( vaddr );

	page_dir_t *dir     = current_page_dir( );
	page_table_t *table = page_current_table_entry( dirent );

	if ( !dir[dirent] ){
		dir[dirent] =
			(page_table_t)add_page_flags( alloc_phys_page( ), PAGE_WRITE );
	}

	table[tableent] = (page_table_t)add_page_flags( raddr, perms );

	return (void *)vaddr;
}

void unmap_page( void *vaddress ){
	unsigned dirent   = page_dir_entry( vaddress );
	unsigned tableent = page_table_entry( vaddress );

	page_dir_t *dir       = current_page_dir( );
	page_table_t *table   = page_current_table_entry( dirent );

	if ( dir[dirent] ){
		if ( table[tableent] ){
			// TODO: check to see if table is entirely empty,
			//       and free the table if so
			void *paddr = (void *)table[tableent];

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

page_dir_t *page_get_kernel_dir( void ){
	return kernel_page_dir;
}

page_dir_t *page_dir_current_phys( void ){
	void *ret = NULL;

	asm volatile ( "mov %%cr3, %0" : "=r"(ret));

	return ret;
}

void set_page_dir( page_dir_t *dir ){
	uintptr_t addr = (uintptr_t)page_phys_addr( dir );
	addr |= PAGE_ARCH_PRESENT | PAGE_ARCH_WRITABLE;

	//debug_printf( "new page dir is at %p\n", addr );

	asm volatile ( "mov %0, %%cr3" :: "r"(addr) );
}

// placed down here so that region allocations don't get used absent-mindedly
// in physical memory allocation code, or something
#include <c4/mm/region.h>

page_dir_t *clone_page_dir( page_dir_t *dir ){
	KASSERT( region_global_is_inited( ));

	page_dir_t *newdir = region_alloc( region_get_global( ));
	KASSERT( newdir != NULL );

	for ( unsigned i = 0; i < 1023; i++ ){
		newdir[i] = dir[i] & ~PAGE_ARCH_ACCESSED;
	}

	// set up recursive mapping for the directory
	newdir[1023] = (page_table_t)page_phys_addr( newdir );

	return newdir;
}
