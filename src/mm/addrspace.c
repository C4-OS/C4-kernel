#include <c4/mm/addrspace.h>
#include <c4/klib/string.h>
#include <c4/debug.h>
#include <c4/common.h>

addr_map_t *addr_map_create( region_t *region ){
	addr_map_t *ret = region_alloc( region );

	memset( ret, 0, sizeof( *ret ));

	ret->region     = region;
	ret->entries    = ADDR_MAP_ENTRIES_PER_PAGE;
	ret->used       = 0;

	debug_printf( "%u\n", sizeof( addr_map_t ));
	debug_printf( "%u\n", sizeof( ret->map ));
	debug_printf( "%d\n", ADDR_MAP_ENTRIES_PER_PAGE );

	return ret;
}

void addr_map_free( addr_map_t *map ){
	if ( map ){
		region_free( map->region, map );
	}
}

void addr_map_dump( addr_map_t *map ){
	debug_printf( "address map @ %p:\n", map );

	for ( unsigned i = 0; i < map->used; i++ ){
		unsigned long start = map->map[i].virtual;
		unsigned long end   = start + map->map[i].size * PAGE_SIZE;

		unsigned long p_start = map->map[i].physical;
		unsigned long p_end   = p_start + map->map[i].size * PAGE_SIZE;

		debug_printf( "  entry %u : %x -> %x\n", i, start, end );
		debug_printf( "           : %x -> %x\n", p_start, p_end );
	}
}

static inline addr_map_t *addr_map_get_root( addr_entry_t *entry ){
	if ( entry ){
		uintptr_t temp = (uintptr_t)entry;

		return (void *)(temp - (temp % PAGE_SIZE));
	}

	return NULL;
}

static inline void addr_map_shift_upwards( addr_map_t *map, unsigned index ){
	for ( unsigned i = map->used; i > index; i-- ){
		map->map[i] = map->map[i - 1];
	}

	memset( map->map + index, 0, sizeof( addr_entry_t ));
	map->used++;
}

static inline void addr_map_shift_downwards( addr_map_t *map, unsigned index ){
	for ( unsigned i = index; i < map->used; i++ ){
		map->map[i] = map->map[i + 1];
	}

	//memset( map->map + index, 0, sizeof( addr_entry_t ));
	map->used--;
}

addr_entry_t *addr_map_lookup( addr_map_t *map, unsigned long address ){
	for ( unsigned i = 0; i < map->used; i++ ){
		unsigned long start = map->map[i].virtual;
		unsigned long end   = start + map->map[i].size * PAGE_SIZE;

		if ( address >= start && address < end ){
			return map->map + i;
		}
	}

	return NULL;
}

addr_entry_t *addr_map_split( addr_map_t *map,
                              addr_entry_t *entry,
                              unsigned long offset )
{
	if ( !entry ){
		return NULL;
	}

	addr_entry_t temp = *entry;

	temp.virtual  += offset * PAGE_SIZE;
	temp.physical += offset * PAGE_SIZE;
	temp.size     -= offset;
	entry->size    = offset;

	return addr_map_insert( map, &temp );
}

void addr_map_remove( addr_map_t *map, addr_entry_t *entry ){
	unsigned index = ((uintptr_t)entry - (uintptr_t)map->map) / sizeof(*entry);

	KASSERT( index >= ADDR_MAP_ENTRIES_PER_PAGE );
	debug_printf( "removing index %u\n", index );

	addr_map_shift_downwards( map, index );
}

addr_entry_t *addr_map_insert( addr_map_t *map, addr_entry_t *entry ){
	if ( map->used >= map->entries || !entry ){
		return NULL;
	}

	int placement = -1;

	// traverse entry list looking for a place to insert the node,
	// leaving -1 in 'placement' if the node should be placed at the end 
	for ( unsigned i = 0; i < map->used; i++ ){
		unsigned long start = map->map[i].virtual;

		if ( entry->virtual < start ){
			placement = i;
			break;
		}
	}

	if ( placement >= 0 ){
		addr_map_shift_upwards( map, placement );

	} else {
		placement = map->used;
		map->used += 1;
	}

	addr_entry_t *ret = map->map + placement;
	*ret = *entry;
	return ret;
}
