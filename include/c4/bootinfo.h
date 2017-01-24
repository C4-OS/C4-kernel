#ifndef _C4_BOOTINFO_H
#define _C4_BOOTINFO_H 1
#include <c4/arch/paging.h>
#include <stdbool.h>

enum {
	BOOTINFO_MAGIC = 0xc0ffee,
};

// constant address of boot information in userspace
static struct bootinfo *bootinfo_addr = (void *)(KERNEL_BASE - PAGE_SIZE);

typedef struct bootinfo {
	// magic number to identify bootinfo struct, defined above
	unsigned magic;

	// semantic versioning info
	struct {
		unsigned major;
		unsigned minor;
		unsigned patch;
	} version;

	// buffer for kernel arguments
	char cmdline[256];

	// total number of pages (not all available to userspace)
	unsigned long num_pages;

	struct {
		// whether there is actually a framebuffer available
		bool      exists;
		// physical address of framebuffer
		uintptr_t addr;
		// framebuffer width
		unsigned  width;
		// framebuffer height
		unsigned  height;
		// bits per pixel
		unsigned  bpp;
	} framebuffer;

} bootinfo_t;

#endif
