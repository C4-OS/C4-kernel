#ifndef _C4_ARCH_PAGING_H
#define _C4_ARCH_PAGING_H 1
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE   0x1000
#define KERNEL_BASE 0xfd000000

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

// converts a page in the 4MB of initial kernel space to its
// corresponding physical page
static inline uintptr_t low_virt_to_phys( uintptr_t addr ){
	return addr - KERNEL_BASE;
}

// inverse of the above, convert an address in the first 4MB of memory
// to the corresponding identity mapped virtual address
static inline uintptr_t low_phys_to_virt( uintptr_t addr ){
	return addr + KERNEL_BASE;
}

static inline bool is_kernel_address( void *addr ){
	uintptr_t temp = (uintptr_t)addr;

	return temp >= KERNEL_BASE;
}

static inline bool is_user_address( void *addr ){
	return !is_kernel_address( addr );
}

void init_paging( void );

#endif
