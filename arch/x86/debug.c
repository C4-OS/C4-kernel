#include <c4/klib/string.h>
#include <c4/arch/paging.h>
#include <c4/arch/ioports.h>
#include <c4/debug.h>
#include <stdbool.h>
#include <stdint.h>

// listing of com io addresses
enum {
	SERIAL_COM1 = 0x3f8,
	SERIAL_COM2 = 0x2f8,
	SERIAL_COM3 = 0x3e8,
	SERIAL_COM4 = 0x2e8,

	SERIAL_DEFAULT = SERIAL_COM1,
};

// address offsets for various info
enum {
	// these two sets of ports alternate depending on the
	// uppermost value of the SERIAL_LINE_CONTROL register
	// this set is used when 0
	SERIAL_DATA              = 0,
	SERIAL_INTERRUPT_ENABLE  = 1,

	// and this when it's 1
	SERIAL_DIVISOR_UPPER     = 0,
	SERIAL_DIVISOR_LOWER     = 1,

	// and the rest are unaffected
	SERIAL_IDENT_CONTROL     = 2,
	SERIAL_LINE_CONTROL      = 3,
	SERIAL_MODEM_CONTROL     = 4,
	SERIAL_LINE_STATUS       = 5,
	SERIAL_MODEM_STATUS      = 6,
	SERIAL_SCRATCH_REGISTER  = 7,
};

// fields in SERIAL_LINE_CONTROL register
enum {
	SERIAL_LINECTRL_DATASIZE_LOWER = (1 << 0),
	SERIAL_LINECTRL_DATASIZE_UPPER = (1 << 1),
	SERIAL_LINECTRL_STOPBITS       = (1 << 2),
	SERIAL_LINECTRL_DLAB           = (1 << 7),
};

// fields in SERIAL_LINE_STATUS register
enum {
	SERIAL_LINESTAT_HAVE_DATA      = (1 << 0),
	SERIAL_LINESTAT_TRANSMIT_EMPTY = (1 << 5),
};

// fields in SERIAL_INTERRUPT_ENABLE register
enum {
	SERIAL_INTERRUPT_HAS_DATA       = (1 << 0),
	SERIAL_INTERRUPT_TRANSMIT_EMPTY = (1 << 1),
	SERIAL_INTERRUPT_ERROR          = (1 << 2),
	SERIAL_INTERRUPT_STATUS_CHANGE  = (1 << 3),
};

static uint8_t read_register( unsigned port, unsigned reg ){
	return inb( port + reg );
}

static void write_register( unsigned port, unsigned reg, uint8_t value ){
	outb( port + reg, value );
}

static void set_divisor( unsigned div ){
	// store current line control register and set DLAB bit
	uint8_t linectrl = read_register( SERIAL_DEFAULT, SERIAL_LINE_CONTROL );
	linectrl |= SERIAL_LINECTRL_DLAB;

	// set divisor registers
	write_register( SERIAL_DEFAULT, SERIAL_DIVISOR_UPPER, div >> 8 );
	write_register( SERIAL_DEFAULT, SERIAL_DIVISOR_LOWER, div &  0xff );

	// reset line control register
	linectrl &= ~SERIAL_LINECTRL_DLAB;
	write_register( SERIAL_DEFAULT, SERIAL_LINE_CONTROL, linectrl );
}

static void set_charsize( unsigned charsize ){
	uint8_t linectrl = read_register( SERIAL_DEFAULT, SERIAL_LINE_CONTROL );
	unsigned val = charsize - 5;

	linectrl |= val;

	write_register( SERIAL_DEFAULT, SERIAL_LINE_CONTROL, linectrl );
}

static void set_interrupts( unsigned interrupts ){
	write_register( SERIAL_DEFAULT, SERIAL_INTERRUPT_ENABLE, interrupts );
}

static bool serial_has_data( void ){
	unsigned temp = read_register( SERIAL_DEFAULT, SERIAL_LINE_STATUS );

	return temp & SERIAL_LINESTAT_HAVE_DATA;
}

static bool serial_can_transmit( void ){
	unsigned temp = read_register( SERIAL_DEFAULT, SERIAL_LINE_STATUS );

	return temp & SERIAL_LINESTAT_TRANSMIT_EMPTY;
}

void debug_putchar( int c ){
	static bool initialized = false;

	if ( !initialized ){
		set_charsize( 8 );    /* 8 bits */
		set_divisor( 1 );     /* 115200 baud */
		set_interrupts( 0 );  /* all disabled */
		initialized = true;
	}

	while ( !serial_can_transmit( ));
	write_register( SERIAL_DEFAULT, SERIAL_DATA, c );
}
