#include <c4/arch/segments.h>
#include <c4/klib/string.h>
#include <stdint.h>
#include <stdbool.h>

static inline void define_descriptor( segment_desc_t *desc,
                                      uint32_t base,
                                      uint32_t limit,
                                      unsigned type,
                                      unsigned privilege )
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
	desc->priv_level  = privilege;
};

static inline void define_task_descriptor( task_state_desc_t *desc,
                                           uint32_t base,
                                           uint32_t limit )
{
	desc->base_low    = base;
	desc->base_high   = base >> 24;

	desc->limit_low   = limit;
	desc->limit_high  = limit >> 16;

	desc->present     = true;
	desc->granularity = SEG_GRANULARITY_BYTE;
	desc->type        = SEG_TASK_DESC;
	desc->priv_level  = ring(0);
}

static inline void init_task_segment( task_seg_t *seg ){
	static unsigned kernel_stack[1024];

	seg->ss_p0  = selector( 2, SEG_TABLE_GDT, ring(0) );
	seg->esp_p0 = (uint32_t)(kernel_stack + 1022);

	seg->cs = selector( 3, SEG_TABLE_GDT, ring(3) );
	seg->ds = selector( 4, SEG_TABLE_GDT, ring(3) );
	seg->ss = seg->es = seg->fs = seg->gs = seg->ds;
}

void init_segment_descs( void ){
	static gdt_ptr_t      gdt;
	static segment_desc_t descripts[6];
    static task_seg_t     task_seg;

	memset( &descripts, 0, sizeof( segment_desc_t[6] ));
    memset( &task_seg,  0, sizeof( task_seg ));

	define_descriptor( descripts + 1, 0x0, 0xfffff,
	                   SEG_CODE | SEG_CODE_READ, ring(0));

	define_descriptor( descripts + 2, 0x0, 0xfffff,
	                   SEG_DATA | SEG_DATA_WRITE, ring(0));

	define_descriptor( descripts + 3, 0x0, 0xfffff,
	                   SEG_CODE | SEG_CODE_READ, ring(3));

	define_descriptor( descripts + 4, 0x0, 0xfffff,
	                   SEG_DATA | SEG_DATA_WRITE, ring(3));

	define_task_descriptor( (void *)(descripts + 5),
	                        (uint32_t)&task_seg,
	                        sizeof( task_seg ));

	gdt.base  = (uint32_t)&descripts;
	gdt.limit = sizeof(descripts) - 1;

	init_task_segment( &task_seg );

	load_gdt( &gdt );
	load_tss( selector( 5, SEG_TABLE_GDT, ring(0) ));
}
