#ifndef _C4_ARCH_INTERRUPTS_H
#define _C4_ARCH_INTERRUPTS_H 1
#include <stdint.h>

// some meta info about interrupts
enum {
	INTERRUPT_MAX = 256,
};

enum {
	INTR_DESC_TYPE_TASK       = 0x5,
	INTR_DESC_TYPE_INTERRUPT  = 0x6,
	INTR_DESC_TYPE_TRAP       = 0x7,
};

enum {
	INTR_SIZE_16BIT = 0,
	INTR_SIZE_32BIT = 1,
};

enum {
	INTERRUPT_DIV_ZERO,
	INTERRUPT_DEBUG,
	INTERRUPT_NMI,
	INTERRUPT_BREAKPOINT,
	INTERRUPT_OVERFLOW,
	INTERRUPT_BOUND,
	INTERRUPT_INVALID_OP,
	INTERRUPT_DEV_UNAVAIL,
	INTERRUPT_DOUBLE_FAULT,
	INTERRUPT_COPROC_SEG_OVERRUN,
	INTERRUPT_INVALID_TSS,
	INTERRUPT_SEG_NOT_PRESENT,
	INTERRUPT_STACK_FAULT,
	INTERRUPT_GEN_PROTECT,
	INTERRUPT_PAGE_FAULT,
	INTERRUPT_FLOAT_ERROR,
	INTERRUPT_ALIGN_CHECK,
	INTERRUPT_MACHINE_CHECK,
	INTERRUPT_SIMD_EXCEPT,
	INTERRUPT_VIRT_EXCEPT,

	// remapped IRQ vectors, see pic.{c,h}
	INTERRUPT_TIMER    = 0x20,
	INTERRUPT_KEYBOARD = 0x21,

	// user syscall interrupt
	INTERRUPT_SYSCALL  = 0x60,
};

typedef struct interrupt_gate {
	unsigned offset_low  : 16;
	unsigned code_select : 16;
	unsigned zeroes      : 8;
	unsigned type        : 3;
	unsigned gate_size   : 1;
    unsigned a_zero      : 1;
	unsigned priv_level  : 2;
	unsigned present     : 1;
	unsigned offset_high : 16;
} __attribute__((packed)) interrupt_gate_t;

typedef struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr_t;

typedef struct interrupt_frame {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t intr_num;
	uint32_t error_num;

	uint32_t eip;
} __attribute__((packed)) interrupt_frame_t;

typedef void (*intr_handler_t)( interrupt_frame_t *frame );

void init_interrupts( void );
void init_cpu_interrupts( void );
void load_idt( idt_ptr_t *ptr );
void register_interrupt( unsigned num, intr_handler_t func );
void interrupt_print_frame( interrupt_frame_t *frame );

#endif
