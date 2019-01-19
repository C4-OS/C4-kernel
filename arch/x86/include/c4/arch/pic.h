// header for 8259A pic
#ifndef _C4_ARCH_PIC_H
#define _C4_ARCH_PIC_H 1
#include <stdint.h>

enum {
	PIC_MASTER   = 0x20,
	PIC_SLAVE    = 0xa0,

	PIC_COMMAND  = 0,
	PIC_DATA     = 1,

	PIC_WORD_PW4  = (1 << 0),
	PIC_WORD_INIT = (1 << 4),
	PIC_WORD_8086 = (1 << 0),

	PIC_COM_END_OF_INTR = 0x20,
};

void remap_pic_vectors( uint8_t master, uint8_t slave );
void remap_pic_vectors_default( void );
void clear_pic_interrupt( void );
void disable_pic(void);

#endif
