#include <c4/mm/addrspace.h>
#include <c4/mm/region.h>
#include <c4/mm/slab.h>
#include <c4/klib/string.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/paging.h>
#include <c4/error.h>
#include <c4/kobject.h>
#include <stdatomic.h>

static slab_t addr_space_slab;
static slab_t phys_frame_slab;
static addr_space_t *kernel_space;

void addr_space_init(void) {
	static bool initialized = false;

	if (!initialized) {
		slab_init_at(&addr_space_slab, region_get_global(),
		              sizeof(addr_space_t), NO_CTOR, NO_DTOR);

		// manually initialize the kernel address space
		kernel_space             = slab_alloc(&addr_space_slab);
		kernel_space->page_dir   = page_get_kernel_dir();
		kernel_space->map        = addr_map_create(region_get_global());
		kernel_space->region     = region_get_global();
		kobject_init(&kernel_space->object, KOBJECT_TYPE_ADDR_SPACE);

		initialized = true;
	}
}

addr_space_t *addr_space_clone(addr_space_t *space) {
	addr_space_t *ret = NULL;
	kobject_lock(&space->object);

	ret = slab_alloc(&addr_space_slab);
	KASSERT(ret != NULL);

	ret->page_dir   = clone_page_dir(space->page_dir);
	ret->map        = addr_map_create(space->region);
	ret->region     = space->region;
	kobject_init(&ret->object, KOBJECT_TYPE_ADDR_SPACE);

	KASSERT(ret->page_dir != NULL);
	KASSERT(ret->map      != NULL);

	memcpy(ret->map, space->map, sizeof(*ret->map));

	kobject_unlock(&space->object);
	return ret;
}

addr_space_t *addr_space_kernel( void ){
	return kernel_space;
}

// TODO: reimplement this once GC is done
/*
void addr_space_free( addr_space_t *space ){
	if ( space && --space->references == 0 ){
		region_free( space->region, space->page_dir );
		addr_map_free( space->map );
		slab_free( &addr_space_slab, space );
	}
}
*/

void addr_space_set( addr_space_t *space ){
	set_page_dir( space->page_dir );
}

void addr_space_make_ent( addr_entry_t *ent,
                          uintptr_t addr,
                          unsigned permissions,
                          phys_frame_t *frame )
{
	ent->addr        = addr;
	ent->frame       = frame;
	ent->permissions = permissions;
}

int addr_space_map( addr_space_t *a,
                    addr_space_t *b,
                    addr_entry_t *ent );

int addr_space_unmap(addr_space_t *space, unsigned long address) {
	kobject_lock(&space->object);
	addr_entry_t *ent = addr_map_lookup(space->map, address);

	if (!ent) {
		kobject_unlock(&space->object);
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	addr_space_remove_map(space, ent);

	kobject_unlock(&space->object);
	return -C4_ERROR_NONE;
}

int addr_space_insert_map(addr_space_t *space, addr_entry_t *ent) {
	phys_frame_t *phys = ent->frame;
	uintptr_t v_start = ent->addr     - (ent->addr     % PAGE_SIZE);
	uintptr_t p_start = phys->address - (phys->address % PAGE_SIZE);

	kobject_lock(&space->object);
	kobject_lock(&phys->object);

	phys_frame_map(ent->frame);
	addr_map_insert(space->map, ent);

	for (uintptr_t page = 0; page < phys->size * PAGE_SIZE; page += PAGE_SIZE)
	{
		void *v = (void *)(v_start + page);
		void *p = (void *)(p_start + page);

		map_phys_page(ent->permissions, v, p);
	}

	kobject_unlock(&phys->object);
	kobject_unlock(&space->object);
	return 0;
}

int addr_space_remove_map(addr_space_t *space, addr_entry_t *ent) {
	phys_frame_t *phys = ent->frame;
	uintptr_t v_start = ent->addr - (ent->addr  % PAGE_SIZE);

	// XXX: not locking `space` since addr_space_unmap will lock it
	// TODO: keep track of recursive locks in mutex_t

	//kobject_lock(&space->object);
	kobject_lock(&phys->object);

	for (uintptr_t page = 0; page < phys->size * PAGE_SIZE; page += PAGE_SIZE)
	{
		void *v = (void *)(v_start + page);

		unmap_page(v);
	}

	phys_frame_unmap(ent->frame);
	addr_map_remove(space->map, ent);

	kobject_unlock(&phys->object);
	//kobject_unlock(&space->object);
	return 0;
}

void phys_frame_init(void) {
	static bool initialized = false;

	if (!initialized) {
		initialized = true;
		slab_init_at(&phys_frame_slab, region_get_global(),
		             sizeof(phys_frame_t), NO_CTOR, NO_DTOR);
	}
}

phys_frame_t *phys_frame_create(uintptr_t addr, size_t size, unsigned flags) {
	phys_frame_t *ret = slab_alloc(&phys_frame_slab);

	ret->address    = addr;
	ret->size       = size;
	ret->flags      = flags;
	ret->mappings   = 0;
	kobject_init(&ret->object, KOBJECT_TYPE_PHYS_MEMORY);

	return ret;
}

void phys_frame_map(phys_frame_t *phys) {
	atomic_fetch_add(&phys->mappings, 1);
}

void phys_frame_unmap(phys_frame_t *phys) {
	KASSERT(phys->mappings != 0);
	atomic_fetch_sub(&phys->mappings, 1);

	if (phys->mappings == 0) {
		debug_printf("=== %u: no mappings, might be able to free this\n", __func__);
	}
}

phys_frame_t *phys_frame_split(phys_frame_t *phys, size_t offset) {
	kobject_lock(&phys->object);

	if (atomic_load(&phys->mappings) != 0) {
		// can't split a frame that is currently mapped
		kobject_unlock(&phys->object);
		return NULL;
	}

	// resulting frame must be smaller than the original frame, and must
	// be non-zero in size
	if (offset == 0 || offset >= phys->size) {
		kobject_unlock(&phys->object);
		return NULL;
	}

	phys_frame_t *ret = phys_frame_create(phys->address + offset * PAGE_SIZE,
	                                      phys->size - offset,
	                                      phys->flags);

	// adjust the size of the original frame
	phys->size = offset;

	kobject_unlock(&phys->object);
	return ret;
}

//phys_frame_t *phys_frame_join( )

addr_map_t *addr_map_create(region_t *region) {
	addr_map_t *ret = region_alloc(region);

	if (ret) {
		memset(ret, 0, sizeof(*ret));

		ret->region     = region;
		ret->entries    = ADDR_MAP_ENTRIES_PER_PAGE;
		ret->used       = 0;

		debug_printf("map size (w/ root):  %u\n", sizeof(addr_map_t));
		debug_printf("map size (w/o root): %u\n", sizeof(ret->map));
		debug_printf("entries per page:    %u\n", ADDR_MAP_ENTRIES_PER_PAGE);
	}

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
		//unsigned long start = map->map[i].virtual;
		//unsigned long end   = start + map->map[i].size * PAGE_SIZE;
		unsigned long start = map->map[i].addr;
		unsigned long end   = start + map->map[i].frame->size * PAGE_SIZE;

		//unsigned long p_start = map->map[i].physical;
		//unsigned long p_end   = p_start + map->map[i].size * PAGE_SIZE;
		unsigned long p_start = map->map[i].frame->address;
		unsigned long p_end   = p_start + map->map[i].frame->size * PAGE_SIZE;

		debug_printf( "  entry %u : %x -> %x\n", i, start, end );
		debug_printf( "          : %x -> %x\n", p_start, p_end );
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
		//unsigned long start = map->map[i].virtual;
		//unsigned long end   = start + map->map[i].size * PAGE_SIZE;
		unsigned long start = map->map[i].addr;
		unsigned long end   = start + map->map[i].frame->size * PAGE_SIZE;

		if ( address >= start && address < end ){
			return map->map + i;
		}
	}

	return NULL;
}

void addr_map_remove( addr_map_t *map, addr_entry_t *entry ){
	unsigned index = ((uintptr_t)entry - (uintptr_t)map->map) / sizeof(*entry);

	KASSERT( index <= ADDR_MAP_ENTRIES_PER_PAGE );
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
		//unsigned long start = map->map[i].virtual;
		unsigned long start = map->map[i].addr;

		//if ( entry->virtual < start ){
		if ( entry->addr < start ){
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
