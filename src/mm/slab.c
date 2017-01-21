#include <c4/mm/region.h>
#include <c4/mm/slab.h>
#include <c4/klib/string.h>
#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/common.h>

static inline void slab_delist_block( slab_blk_t *blk ){
	if ( blk->list ){
		if ( blk->prev ){
			blk->prev->next = blk->next;
		}

		if ( blk->next ){
			blk->next->prev = blk->prev;
		}

		if ( blk->list->first == blk ){
			blk->list->first = blk->next;
		}

		blk->list->size--;
	}
}

static inline void slab_insert_block( slab_blk_t *blk, slab_list_t *list ){
	if ( !list->first ){
		list->first = blk;
		blk->next = NULL;

	} else {
		list->first->prev = blk;
		blk->next = list->first;
		list->first = blk;
	}

	blk->prev = (void *)0;
	blk->list = list;
	list->size++;
}

static inline void slab_move_block( slab_blk_t *blk, slab_list_t *list ){
	if ( list ){
		slab_delist_block( blk );
		slab_insert_block( blk, list );
	}
}

static inline slab_blk_t *slab_pop_block( slab_list_t *list ){
	slab_blk_t *temp = list->first;

	slab_delist_block( temp );

	return temp;
}

static inline slab_blk_t *slab_alloc_block( slab_t *slab ){
	slab_blk_t *blk = region_alloc( slab->region );

	blk->magic = 0xabadc0de;
	blk->map   = 0;
	blk->slab  = slab;
	blk->list  = (void *)0;

	bitmap_set( &blk->map, 0 );
	slab->total_pages++;

	return blk;
}

static inline void slab_dealloc_free_block( slab_t *slab ){
	if ( slab->free.size > MAX_FREE_SLABS ){
		slab_blk_t *block = slab_pop_block( &slab->free );

		region_free( slab->region, block );
	}
}

void *slab_alloc( slab_t *slab ){
	bool      retried = false;
	slab_blk_t *block = (void *)0;

retry:
	if ( slab->partial.first ){
		block = slab->partial.first;

	} else if ( slab->free.first ){
		block = slab->free.first;

	} else {
		slab_blk_t *temp = slab_alloc_block( slab );

		slab_move_block( temp, &slab->free );
		if ( !retried ){
			//debug_printf( "allocated new block %p, retrying...\n", temp );
			retried = true;
			goto retry;
		}
	}

	if ( block ){
		int n = bitmap_first_free( &block->map, BITMAP_BPS );
		uintptr_t addr = (uintptr_t)block + n * slab->obj_size;

		// leaving some debugging statements commented out for further
		// planned work on slab allocator
		/*
		debug_printf( "bitmap: 0x%x, location: %u, pages: %u\n",
			block->map, n, slab->total_pages );
		 */

		bitmap_set( &block->map, n );

		if ( block->map == BITMAP_ENT_FULL ){
			slab_move_block( block, &slab->full );

		} else if ( block->list == &slab->free ){
			slab_move_block( block, &slab->partial );
		}

		// run the constructor, if there is one
		slab->ctor? slab->ctor( (void *)addr ) : 0;

		return (void *)addr;
	}

	return (void *)0;
}

void slab_free( slab_t *slab, void *ptr ){
	if ( ptr ){
		uintptr_t temp    = (uintptr_t)ptr;
		uintptr_t addr    = temp / PAGE_SIZE * PAGE_SIZE;
		uintptr_t pos     = (temp - addr) / (PAGE_SIZE / BITMAP_BPS);
		slab_blk_t *block = (void *)addr;

		if ( block->magic != MAGIC ){
			debug_printf( "bad magic! corrupted block or bad address at %p?", ptr );
			return;
		}

		// run the destructor, if there is one
		slab->dtor? slab->dtor( (void *)addr ) : 0;
		bitmap_unset( &block->map, pos );

		// test to see if there are any entries besides the first
		// information block, and stuff it into the free list if not
		if ( block->map == 1 ){
			slab_move_block( block, &slab->free );

			slab_dealloc_free_block( slab );

		} else if ( block->list == &slab->full ){
			slab_move_block( block, &slab->partial );
		}
	}
}

static inline unsigned slab_adjust_size( unsigned size ){
	unsigned min_size = PAGE_SIZE / BITMAP_BPS;
	unsigned ret;

	if ( size >= min_size ){
		debug_printf( "warning: size of %u requested, "
				"but only sizes less than %u supported atm",
				size, min_size );

		ret  = (size / min_size) * min_size;
		ret += size % min_size;

	} else {
		ret = min_size;
	}

	return ret;
}

slab_t *slab_init_at( slab_t   *slab,
                      region_t *region,
					  unsigned obj_size,
                      void (*ctor)(void *ptr),
                      void (*dtor)(void *ptr) )
{
	memset( slab, 0, sizeof( slab_t ));

	slab->total_pages = 0;
	slab->obj_size    = slab_adjust_size( obj_size );
	slab->ctor        = ctor;
	slab->dtor        = dtor;
	slab->region      = region;

	return slab;
}
