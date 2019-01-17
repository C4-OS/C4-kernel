#include <c4/arch/interrupts.h>
#include <c4/arch/segments.h>
#include <c4/arch/syscall.h>
#include <c4/klib/string.h>
#include <c4/debug.h>
#include <c4/interrupts.h>
#include <stdbool.h>

static interrupt_gate_t intr_table[256];
static intr_handler_t   intr_handlers[256];
static unsigned long    intr_stats[256];
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
void register_interrupt( unsigned num, intr_handler_t func ){
	intr_handlers[num] = func;
}

void interrupt_print_frame( interrupt_frame_t *frame ){
	debug_printf( " => edi: 0x%x \n"
				  " => esi: 0x%x \n"
				  " => ebp: 0x%x \n"
				  " => esp: 0x%x \n"
				  " => ebx: 0x%x \n"
				  " => edx: 0x%x \n"
				  " => ecx: 0x%x \n"
				  " => eax: 0x%x \n"
				  " => eip: 0x%x \n"
				  " => interrupt: %u \n"
				  " => error:     %u \n"
				  ,
				  frame->edi, frame->esi, frame->ebp, frame->esp,
				  frame->ebx, frame->edx, frame->ecx, frame->eax,
				  frame->eip, frame->intr_num, frame->error_num
				  );
}

static void test_handler( interrupt_frame_t *frame ){
	debug_printf( "interrupt handler called for %u\n", frame->intr_num );

	//interrupt_print_frame( frame );
	//for ( ;; );
}

static void gen_protect_fault( interrupt_frame_t *frame ){
	debug_printf( "=== general protection fault! ===\n" );
	debug_printf( "=== error: 0b%b ===\n", frame->error_num );

	interrupt_print_frame( frame );

	for (;;);
}

static void double_fault_handler( interrupt_frame_t *frame ){
	debug_printf( "=== double fault! ===\n" );
	debug_printf( "=== error: 0b%b ===\n", frame->error_num );

	interrupt_print_frame( frame );

	for (;;);
}

void init_interrupts( void ){
	uint32_t *stubs = &isr_stubs;

	debug_puts( "(intr. setup function) " );
	memset( intr_table,    0, sizeof( intr_table ));
	memset( intr_handlers, 0, sizeof( intr_handlers ));

	for ( unsigned i = 0; i < 256; i++ ){
		define_intr_descript( intr_table + i, stubs[i], ring(0) );
	}

	// define the user syscall interrupt descriptor, overwriting the
	// descriptor set above
	define_intr_descript( intr_table + INTERRUPT_SYSCALL,
	                      stubs[INTERRUPT_SYSCALL],
	                      ring(3) );

	register_interrupt( INTERRUPT_DEBUG,        test_handler );
	register_interrupt( INTERRUPT_GEN_PROTECT,  gen_protect_fault );
	register_interrupt( INTERRUPT_DOUBLE_FAULT, double_fault_handler );
	register_interrupt( INTERRUPT_SYSCALL,      syscall_handler );

	idtr.base  = (uint32_t)intr_table;
	idtr.limit = sizeof(interrupt_gate_t[256]) - 1;

	load_idt( &idtr );
}

void init_cpu_interrupts( void ){
	load_idt( &idtr );
}

void isr_dispatch( interrupt_frame_t *frame ){
	intr_stats[frame->intr_num]++;

	// call the general interrupt callback handler, for interrupt operations
	// which aren't implementation-specific
	bool handled = interrupt_callback( frame->intr_num, 0 );

	if ( intr_handlers[frame->intr_num] ){
		intr_handlers[frame->intr_num]( frame );
		handled = true;
	}

	if ( !handled ){
		debug_printf( "note: have unhandled interrupt %u, call %u\n",
		              frame->intr_num, intr_stats[frame->intr_num] );
	}
}

#include <c4/arch/pic.h>
#include <c4/arch/apic.h>
void irq_dispatch( interrupt_frame_t *frame ){
	// interrupt stubs in idt.s disable maskable interrupts,
	// so setting end of interrupt here won't result in nested interrupts
	if (apic_is_enabled()) {
		apic_end_of_interrupt();

	} else {
		clear_pic_interrupt();
	}

	// then proceed to handle things like a normal isr
	isr_dispatch(frame);
}
