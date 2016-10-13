#ifndef _C4_BITMAP_H
#define _C4_BITMAP_H 1
#include <stdbool.h>
#include <stdint.h>

enum {
	// bits per section
	BITMAP_BPS  = 32,
	// value of a section when it's full
	BITMAP_ENT_FULL = 0xffffffff,
};

typedef uint32_t bitmap_ent_t;

static inline bool bitmap_get( bitmap_ent_t *bitmap, unsigned i ){
	unsigned bitindex = i / 8;
	unsigned offset   = i % 8;

	return (bitmap[bitindex] >> offset) & 1;
}

static inline void bitmap_set( bitmap_ent_t *bitmap, unsigned i ){
	unsigned bitindex = i / BITMAP_BPS;
	unsigned offset   = i % BITMAP_BPS;
	bitmap_ent_t bitvalue = bitmap[bitindex];

	bitmap[bitindex] = bitvalue | (1 << offset);
}

static inline void bitmap_unset( bitmap_ent_t *bitmap, unsigned i ){
	unsigned bitindex = i / BITMAP_BPS;
	unsigned offset   = i % BITMAP_BPS;
	bitmap_ent_t bitvalue = bitmap[bitindex];

	bitmap[bitindex] = bitvalue & ~(1 << offset);
}

#endif
