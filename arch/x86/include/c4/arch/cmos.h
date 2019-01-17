#ifndef _MP_CMOS_H
#define _MP_CMOS_H 1
#include <c4/arch/ioports.h>

enum {
	CMOS_PORT_SELECT = 0x70,
	CMOS_PORT_DATA   = 0x71,
};

enum {
	CMOS_REG_RESET = 0xf,
};

enum {
	CMOS_RESET_POWERON = 0x0,
	CMOS_RESET_JUMP    = 0xa,
};

static inline void cmos_select( uint8_t reg, bool disable_nmi ){
	outb( CMOS_PORT_SELECT, (disable_nmi << 7) | reg );
}

static inline uint8_t cmos_read( uint8_t reg ){
	cmos_select( reg, false );
	return inb( CMOS_PORT_DATA );
}

static inline void cmos_write( uint8_t reg, uint8_t value ){
	cmos_select( reg, false );
	outb( CMOS_PORT_DATA, value );
}

#endif
