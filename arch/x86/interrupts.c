#include <c4/arch/interrupts.h>
#include <c4/arch/segments.h>
#include <c4/klib/string.h>
#include <c4/debug.h>
#include <stdbool.h>

static interrupt_gate_t intr_table[256];
static intr_handler_t   intr_handlers[256];
static idt_ptr_t idtr;

// defined in idt.s
extern uint32_t isr_stubs;

static inline void define_intr_descript( interrupt_gate_t *gate,
                                         uint32_t offset,
                                         uint8_t  priv_level )
{
	memset( gate, 0, sizeof( interrupt_gate_t ));

	gate->offset_low  = offset;
	gate->offset_high = offset >> 16;
	gate->code_select = selector(1, SEG_TABLE_GDT, ring(0));
	gate->type        = INTR_DESC_TYPE_INTERRUPT;
	gate->gate_size   = INTR_SIZE_32BIT;
	gate->priv_level  = priv_level;
	gate->present     = true;
}

// TODO: have linked list of handlers to call for a given interrupt
static inline void register_interrupt( unsigned num, intr_handler_t func ){
	intr_handlers[num] = func;
}

void test_handler( interrupt_frame_t *frame ){
	debug_printf( "interrupt handler called for %u\n", frame->intr_num );
}

void init_interrupts( void ){
	uint32_t *stubs = &isr_stubs;

	debug_puts( "(intr. setup function) " );
	memset( intr_table,    0, sizeof( intr_table ));
	memset( intr_handlers, 0, sizeof( intr_handlers ));

	for ( unsigned i = 0; i < 32; i++ ){
		define_intr_descript( intr_table + i, stubs[i], ring(0) );
	}

	register_interrupt( 1, test_handler );
	register_interrupt( 5, test_handler );

	idtr.base  = (uint32_t)intr_table;
	idtr.limit = sizeof(interrupt_gate_t[256]) - 1;

	load_idt( &idtr );
}

void isr_dispatch( interrupt_frame_t *frame ){
	debug_printf( "interrupts are working! frame at %p\n", frame );
	debug_printf( "got interrupt %u\n", frame->intr_num );

	if ( intr_handlers[frame->intr_num] ){
		intr_handlers[frame->intr_num]( frame );

	} else {
		debug_printf( "have unhandled interrupt %u\n", frame->intr_num );
	}
}
