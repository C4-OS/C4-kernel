#ifndef _C4_ARCH_PAGING_H
#define _C4_ARCH_PAGING_H 1
#include <stdint.h>

#define PAGE_SIZE 0x1000

enum {
	PAGE_ARCH_PRESENT    = 1 << 0,
	PAGE_ARCH_WRITABLE   = 1 << 1,
	PAGE_ARCH_SUPERVISOR = 1 << 2,
	PAGE_ARCH_ACCESSED   = 1 << 5,
	PAGE_ARCH_4MB_ENTRY  = 1 << 7,

	PAGE_ARCH_ALL_FLAGS  = 0xfff,
};

typedef uint32_t page_dir_t;
typedef uint32_t page_table_t;

static inline unsigned page_dir_entry( void *vaddress ){
	uintptr_t temp = (uintptr_t)vaddress;

	// return upper 10 bits (31:22)
	return temp >> 22;
}

static inline unsigned page_table_entry( void *vaddress ){
	uintptr_t temp = (uintptr_t)vaddress;

	// return only bits 21:12
	return (temp >> 12) & 0x3ff;
}

void init_paging( void );

#endif
