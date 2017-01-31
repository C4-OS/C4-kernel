// Generic paging interface
#ifndef _C4_PAGING_H
#define _C4_PAGING_H 1
#include <c4/arch/paging.h>
#include <stdint.h>

typedef enum {
	PAGE_READ       = 1,
	PAGE_WRITE      = 2,
	PAGE_EXECUTE    = 4,
	PAGE_SUPERVISOR = 8,
} page_flags_t;

// TODO: change function names from <verb>-<thing>[-thing] to
//       <thing>-<verb>[-thing] to match the rest of the kernel functions
//
//       so 'page_map_phys' instead of 'map_phys_page'
void *map_page( unsigned permissions, void *vaddress );
void *map_phys_page( unsigned perm, void *vaddr, void *raddr );
void unmap_page( void *vaddress );

void page_fault_message( uintptr_t address, uintptr_t ip, unsigned perms );
page_dir_t *current_page_dir( void );
page_dir_t *page_get_kernel_dir( void );
page_dir_t *clone_page_dir( page_dir_t *dir );
void        set_page_dir( page_dir_t *dir );
void        page_reserve_phys_range( uintptr_t start, uintptr_t end );
void       *page_phys_addr( void *vaddress );

#endif
