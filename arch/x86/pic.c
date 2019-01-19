#include <c4/arch/pic.h>
#include <c4/arch/ioports.h>

void remap_pic_vectors( uint8_t master, uint8_t slave ){
	uint8_t masks[2];

	masks[0] = inb( PIC_MASTER | PIC_DATA );
	masks[1] = inb( PIC_SLAVE  | PIC_DATA );

	// start initialization
	outb( PIC_MASTER | PIC_COMMAND, PIC_WORD_INIT | PIC_WORD_PW4 );
	outb( PIC_SLAVE  | PIC_COMMAND, PIC_WORD_INIT | PIC_WORD_PW4 );

	// set new interrupt vector offsets
	outb( PIC_MASTER | PIC_DATA, master );
	outb( PIC_SLAVE  | PIC_DATA, slave );

	// tell controllers where things are
	outb( PIC_MASTER | PIC_DATA, 4 );
	outb( PIC_SLAVE  | PIC_DATA, 2 );

	outb( PIC_MASTER | PIC_DATA, PIC_WORD_8086 );
	outb( PIC_SLAVE  | PIC_DATA, PIC_WORD_8086 );

	// restore original masks
	outb( PIC_MASTER | PIC_DATA, masks[0] );
	outb( PIC_SLAVE  | PIC_DATA, masks[1] );
}

void remap_pic_vectors_default( void ){
	remap_pic_vectors( 0x20, 0x28 );
}

void clear_pic_interrupt( void ){
	outb( PIC_MASTER | PIC_COMMAND, PIC_COM_END_OF_INTR );
	outb( PIC_SLAVE  | PIC_COMMAND, PIC_COM_END_OF_INTR );
}

void disable_pic(void){
	asm volatile ("mov $0xff, %al;"
	              "outb %al, $0xa1;"
	              "outb %al, $0x21;");
}
