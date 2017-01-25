#ifndef _C4_ADDR_SPACE_H
#define _C4_ADDR_SPACE_H 1
#include <c4/paging.h>
#include <c4/mm/region.h>
#include <stdbool.h>
#include <stdint.h>

// default location for address space memory maps, for use by userspace
// threads
//
// TODO: consider making this arch-specific, with eg. a platform-specific
//       layout.h header to describe where things should be placed,
//       and so things aren't overlapped by mistake
#define ADDR_MAP_ADDR ((void *)(KERNEL_BASE - PAGE_SIZE * 2))

typedef struct addr_node {
	unsigned long virtual;
	unsigned long physical;
	unsigned long size;

	unsigned references   : 22;
	unsigned source       : 2;
	unsigned permissions  : 3;
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
void          addr_space_map_self( addr_space_t *space, void *addr );

int addr_space_map( addr_space_t *a,
                    addr_space_t *b,
                    addr_entry_t *ent );

int addr_space_grant( addr_space_t *a,
                      addr_space_t *b,
                      addr_entry_t *ent );

int addr_space_unmap( addr_space_t *space, unsigned long address );
int addr_space_insert_map( addr_space_t *space, addr_entry_t *ent );
int addr_space_remove_map( addr_space_t *space, addr_entry_t *ent );

addr_map_t *addr_map_create( region_t *region );
void        addr_map_free( addr_map_t *map );
void        addr_map_dump( addr_map_t *map );

addr_entry_t *addr_map_lookup( addr_map_t *map, unsigned long address );
addr_entry_t *addr_map_split( addr_map_t *map,
                              addr_entry_t *entry,
                              unsigned long offset );
addr_entry_t *addr_map_carve( addr_map_t *map, addr_entry_t *entry );
void          addr_map_remove( addr_map_t *map, addr_entry_t *entry );
addr_entry_t *addr_map_insert( addr_map_t *map, addr_entry_t *entry );

#endif
