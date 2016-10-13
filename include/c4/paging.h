// Generic paging interface
#ifndef _C4_PAGING_H
#define _C4_PAGING_H 1
#include <c4/arch/paging.h>

typedef enum {
	PAGE_READ       = 1,
	PAGE_WRITE      = 2,
	PAGE_EXECUTE    = 4,
	PAGE_SUPERVISOR = 8,
} page_flags_t;

void *map_page( unsigned permissions, void *vaddress );
void *map_phys_page( unsigned perm, void *vaddr, void *raddr );
void unmap_page( void *vaddress );

page_dir_t *current_page_dir( void );

#endif
