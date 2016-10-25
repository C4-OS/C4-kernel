#ifndef _C4_REGION_H
#define _C4_REGION_H 1
#include <c4/klib/bitmap.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct region {
	bitmap_ent_t *bitmap;
	void         *vaddress;

	unsigned num_pages;
	unsigned available;
	unsigned page_flags;
} region_t;

void *region_alloc( region_t *region );
void  region_free( region_t *region, void *page );

region_t *region_init_at( region_t     *region,
                          void         *vaddress,
                          bitmap_ent_t *bitmap,
                          unsigned     num_pages,
                          unsigned     page_flags );

void region_init_global( void );
region_t *region_get_global( void );
bool region_global_is_inited( void );

#endif
