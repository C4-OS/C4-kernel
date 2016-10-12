#ifndef _C4_BITMAP_H
#define _C4_BITMAP_H 1
#include <stdbool.h>
#include <stdint.h>

static inline bool bitmap_get( uint8_t *bitmap, unsigned i ){
	unsigned bitindex = i / 8;
	unsigned offset   = i % 8;

	return (bitmap[bitindex] >> offset) & 1;
}

static inline void bitmap_set( uint8_t *bitmap, unsigned i ){
	unsigned bitindex = i / 8;
	unsigned offset   = i % 8;
	uint8_t bitvalue = bitmap[bitindex];

	bitmap[bitindex] = bitvalue | (1 << offset);
}

static inline void bitmap_unset( uint8_t *bitmap, unsigned i ){
	unsigned bitindex = i / 8;
	unsigned offset   = i % 8;
	uint8_t bitvalue = bitmap[bitindex];

	bitmap[bitindex] = bitvalue & ~(1 << offset);
}

#endif
