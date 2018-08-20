#ifndef _C4_ADDR_SPACE_H
#define _C4_ADDR_SPACE_H 1
#include <c4/paging.h>
#include <c4/mm/region.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
	PHYS_FRAME_FLAG_NONE = 0,
	// TODO: doxygen markup here
	// signals that this frame can't be recollected, e.g. frames created by
	// device drivers which access memory mapped I/O and what not
	PHYS_FRAME_FLAG_NONFREEABLE = (1 << 0),
};

// structure representing a linear block of physical memory
typedef struct phys_frame {
	// TODO locking, GC structure, etc
	uintptr_t address;
	size_t    size;
	unsigned  flags;
	unsigned  references;
	unsigned  mappings;
} phys_frame_t;

// default location for address space memory maps, for use by userspace
// threads
//
// TODO: consider making this arch-specific, with eg. a platform-specific
//       layout.h header to describe where things should be placed,
//       and so things aren't overlapped by mistake
#define ADDR_MAP_ADDR ((void *)(KERNEL_BASE - PAGE_SIZE * 2))

typedef struct addr_node {
	uintptr_t     addr;
	phys_frame_t *frame;
	//size_t        size;
	unsigned      permissions;
} __attribute__((packed)) addr_entry_t;

enum {
	ADDR_MAP_ENTRIES_PER_PAGE = PAGE_SIZE / sizeof( addr_entry_t ) - 1,
};

typedef struct addr_map {
	union {
		// make the root info block take up one entry space
		addr_entry_t filler;

		struct {
			region_t *region;
			uint16_t entries;
			uint16_t used;
		};
	};

	// TODO: consider making a linked list of maps, so there's no arbitrary
	//       limit on the number of mappings per task
	addr_entry_t map[ADDR_MAP_ENTRIES_PER_PAGE];
} addr_map_t;

typedef struct addr_space {
	page_dir_t *page_dir;
	addr_map_t *map;
	region_t   *region;

	unsigned references;
} addr_space_t;

void addr_space_init( void );

addr_space_t *addr_space_clone( addr_space_t *space );
addr_space_t *addr_space_reference( addr_space_t *space );
addr_space_t *addr_space_kernel( void );
void          addr_space_free( addr_space_t *space );
void          addr_space_set( addr_space_t *space );
void          addr_space_make_ent( addr_entry_t *ent,
                                   uintptr_t addr,
                                   unsigned permissions,
                                   phys_frame_t *frame );

int addr_space_map( addr_space_t *a,
                    addr_space_t *b,
                    addr_entry_t *ent );

int addr_space_unmap( addr_space_t *space, unsigned long address );
int addr_space_insert_map( addr_space_t *space, addr_entry_t *ent );
int addr_space_remove_map( addr_space_t *space, addr_entry_t *ent );

void          phys_frame_init( void );
phys_frame_t *phys_frame_create( uintptr_t addr, size_t size, unsigned flags );
void          phys_frame_map( phys_frame_t *phys );
void          phys_frame_unmap( phys_frame_t *phys );
phys_frame_t *phys_frame_split( phys_frame_t *phys, size_t offset );

addr_map_t *addr_map_create( region_t *region );
void        addr_map_free( addr_map_t *map );
void        addr_map_dump( addr_map_t *map );

addr_entry_t *addr_map_lookup( addr_map_t *map, unsigned long address );
void          addr_map_remove( addr_map_t *map, addr_entry_t *entry );
addr_entry_t *addr_map_insert( addr_map_t *map, addr_entry_t *entry );

#endif
