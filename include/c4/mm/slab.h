#ifndef _C4_SLAB_H
#define _C4_SLAB_H 1
#include <c4/mm/region.h>
#include <c4/syncronization.h>

#define MAGIC          0xabadc0de
#define MAX_FREE_SLABS 2
#define NO_CTOR        ((void *)0)
#define NO_DTOR        ((void *)0)

typedef struct slab slab_t;
typedef struct slab_list slab_list_t;

typedef struct slab_blk {
	uint32_t     magic;
	bitmap_ent_t map;
	slab_t      *slab;

	struct slab_blk *prev;
	struct slab_blk *next;
    slab_list_t *list;
} slab_blk_t;

typedef struct slab_list {
	slab_blk_t *first;
	unsigned size;
} slab_list_t;

typedef struct slab {
	lock_t lock;

	slab_list_t free;
	slab_list_t partial;
	slab_list_t full;
	region_t *region;

	void (*ctor)(void *ptr);
	void (*dtor)(void *ptr);

	unsigned obj_size;
	unsigned total_pages;
} slab_t;

void *slab_alloc( slab_t *slab );
void  slab_free( slab_t *slab, void *ptr );

slab_t *slab_init_at( slab_t *slab,
                      region_t *region,
                      unsigned obj_size,
                      void (*ctor)(void *ptr),
                      void (*dtor)(void *ptr) );

#endif
