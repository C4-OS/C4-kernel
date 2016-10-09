#ifndef _C4_ARCH_SEGMENTS_H
#define _C4_ARCH_SEGMENTS_H 1
#include <stdint.h>

enum {
	SEG_GRANULARITY_BYTE = 0,
	SEG_GRANULARITY_PAGE = 1,
};

enum {
	SEG_OP_SIZE_16BIT = 0,
	SEG_OP_SIZE_32BIT = 1,
};

enum {
	SEG_DATA          = 0x0,
	SEG_CODE          = 0x8,
	SEG_ACCESSED      = 0x1,

	SEG_CODE_READ     = 0x2,
	SEG_CODE_CONFORM  = 0x4,

	SEG_DATA_WRITE    = 0x2,
	SEG_DATA_EXPAND   = 0x4,
};

typedef struct segment_desc {
	unsigned limit_low   : 16;
	unsigned base_low    : 16;
	unsigned base_mid    : 8;
	unsigned type        : 4;
	unsigned is_data     : 1;
	unsigned priv_level  : 2;
	unsigned present     : 1;
	unsigned limit_high  : 4;
	unsigned unused      : 1;
	unsigned is_longmode : 1;
	unsigned op_size     : 1;
	unsigned granularity : 1;
	unsigned base_high   : 8;
} __attribute__((packed)) segment_desc_t;

typedef struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

void init_segment_descs( void );
void load_gdt( void *ptr );

#endif
