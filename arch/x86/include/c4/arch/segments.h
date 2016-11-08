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

	SEG_TASK_DESC     = 0x9,
};

enum {
	SEG_TABLE_GDT = 0,
	SEG_TABLE_LDT = 1,
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

typedef struct task_state_seg {
	unsigned prev_task   : 16;
	unsigned reserved_0  : 16;
	unsigned esp_p0      : 32;

	unsigned ss_p0       : 16;
	unsigned reserved_1  : 16;
	unsigned esp_p1      : 32;

	unsigned ss_p1       : 16;
	unsigned reserved_2  : 16;
	unsigned esp_p2      : 32;

	unsigned ss_p2       : 16;
	unsigned reserved_3  : 16;

	unsigned cr3         : 32;
	unsigned eip         : 32;
	unsigned eflags      : 32;
	unsigned eax         : 32;
	unsigned ecx         : 32;
	unsigned edx         : 32;
	unsigned ebx         : 32;
	unsigned esp         : 32;
	unsigned ebp         : 32;
	unsigned esi         : 32;
	unsigned edi         : 32;

	unsigned es          : 16;
	unsigned reserved_4  : 16;

	unsigned cs          : 16;
	unsigned reserved_5  : 16;

	unsigned ss          : 16;
	unsigned reserved_6  : 16;

	unsigned ds          : 16;
	unsigned reserved_7  : 16;

	unsigned fs          : 16;
	unsigned reserved_8  : 16;

	unsigned gs          : 16;
	unsigned reserved_9  : 16;

	unsigned ldt         : 16;
	unsigned reserved_10 : 16;

	unsigned debug_trap  : 1;
	unsigned reserved_11 : 15;

	unsigned iomap_base  : 16;
} __attribute__((packed)) task_seg_t;

typedef struct task_state_desc {
	unsigned limit_low   : 16;
	unsigned base_low    : 24;

	unsigned type        : 4;

	unsigned always_0a   : 1;
	unsigned priv_level  : 2;
	unsigned present     : 1;
	unsigned limit_high  : 4;
	unsigned available   : 1;
	unsigned always_0b   : 2;
	unsigned granularity : 1;
	unsigned base_high   : 8;
} __attribute__((packed)) task_state_desc_t;

// C implementation of segment selector macro in gdt.s
static inline uint16_t selector( unsigned index, unsigned table, unsigned priv )
{
	return (index << 3) | (table << 2) | priv;
}

// and again the same thing as the ring() macro in gdt.s
static inline unsigned ring(unsigned n){
	return n;
}

void init_segment_descs( void );
void load_gdt( void *ptr );
void load_tss( uint32_t seg );

void set_user_stack( void *addr );
void kernel_stack_set( void *addr );
void *kernel_stack_get( void );

#endif
