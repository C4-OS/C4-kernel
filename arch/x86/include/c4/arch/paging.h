#ifndef _C4_ARCH_PAGING_H
#define _C4_ARCH_PAGING_H 1
#include <stdint.h>

#define PAGE_SIZE 0x1000

enum {
	PAGE_ARCH_PRESENT    = 1,
	PAGE_ARCH_WRITABLE   = 2,
	PAGE_ARCH_SUPERVISOR = 4,
};

typedef uint32_t page_dir_t;

void init_paging( void );

#endif
