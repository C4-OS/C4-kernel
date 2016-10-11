#ifndef _C4_ARCH_INTERRUPTS_H
#define _C4_ARCH_INTERRUPTS_H 1
#include <stdint.h>

enum {
	INTR_DESC_TYPE_TASK       = 0x5,
	INTR_DESC_TYPE_INTERRUPT  = 0x6,
	INTR_DESC_TYPE_TRAP       = 0x7,
};

enum {
	INTR_SIZE_16BIT = 0,
	INTR_SIZE_32BIT = 1,
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
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint8_t intr_num;
	uint8_t error_no;
} interrupt_frame_t;

void init_interrupts( void );
void load_idt( idt_ptr_t *ptr );

#endif
