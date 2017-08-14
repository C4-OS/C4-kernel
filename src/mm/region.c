#include <c4/mm/region.h>
#include <c4/paging.h>
#include <stdint.h>
#include <stdbool.h>

void *region_alloc( region_t *region ){
	if ( region->available == 0 ){
		return (void *)0;
	}

	int n = bitmap_first_free( region->bitmap, region->num_pages );
	void *addr = (uint8_t *)region->vaddress + n * PAGE_SIZE;

	bitmap_set( region->bitmap, n );
	region->available--;

	// Don't map the page if this region is keeping track of physical memory
	if ( region->mode == REGION_MODE_VIRTUAL ){
		map_page( region->page_flags, addr );
	}

	return addr;
}

void region_free( region_t *region, void *page ){
	uintptr_t n = (uintptr_t)(page - region->vaddress) / PAGE_SIZE;

	// Same as above, don't try to unmap a physical address
	if ( region->mode == REGION_MODE_VIRTUAL ){
		unmap_page( page );
	}

	bitmap_unset( region->bitmap, n );
	region->available++;
}

region_t *region_init_at( region_t     *region,
                          void         *vaddress,
                          bitmap_ent_t *bitmap,
                          unsigned     num_pages,
                          unsigned     page_flags,
                          unsigned     mode )
{
	region->bitmap     = bitmap;
	region->vaddress   = vaddress;
	region->num_pages  = num_pages;
	region->available  = num_pages;
	region->page_flags = page_flags;
	region->mode       = mode;

	// make sure every entry in the bitmap is clear
	for ( unsigned i = 0; i < num_pages; i++ ){
		bitmap_unset( bitmap, i );
	}

	return region;
}

static bitmap_ent_t region_map[32];
static region_t     global_region;
static bool         initialized = false;

void region_init_global( void *addr ){
	if ( !initialized ){
		//region_init_at( &global_region, (void*)0xc1000000, region_map,
		region_init_at( &global_region, addr, region_map,
		                32 * BITMAP_BPS,
		                PAGE_READ | PAGE_WRITE | PAGE_SUPERVISOR,
		                REGION_MODE_VIRTUAL );

		initialized = true;
	}
}

region_t *region_get_global( void ){
	return &global_region;
}

bool region_global_is_inited( void ){
	return initialized;
}
