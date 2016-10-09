#include <c4/arch/segments.h>
#include <c4/klib/string.h>
#include <stdint.h>
#include <stdbool.h>

static inline void define_descriptor( segment_desc_t *desc,
                                      uint32_t base,
                                      uint32_t limit,
                                      unsigned type )
{
	desc->limit_low   = limit;
	desc->limit_high  = limit >> 16;

	desc->base_low    = base;
	desc->base_mid    = base >> 16;
	desc->base_high   = base >> 24;

	desc->granularity = SEG_GRANULARITY_PAGE;
	desc->op_size     = SEG_OP_SIZE_32BIT;
	desc->present     = true;
	desc->is_data     = true;
	desc->is_longmode = false;
	desc->type        = type;
	desc->priv_level  = 0;
};

void init_segment_descs( void ){
	static gdt_ptr_t      gdt;
	static segment_desc_t descripts[6];

	memset( &descripts, 0, sizeof(segment_desc_t[4]));
	define_descriptor( descripts + 1, 0x0, 0xfffff, SEG_CODE | SEG_CODE_READ );
	define_descriptor( descripts + 2, 0x0, 0xfffff, SEG_DATA | SEG_DATA_WRITE );

	gdt.base  = (uint32_t)&descripts;
	gdt.limit = sizeof(descripts) - 1;

	load_gdt( &gdt );
}
