#ifndef _MP_PIT_H
#define _MP_PIT_H 1
#include <stdint.h>

enum {
	PIT_CHAN0   = 0x40,
	PIT_CHAN1   = 0x41,
	PIT_CHAN2   = 0x42,
	PIT_COMMAND = 0x43,
};

enum {
	PIT_CMD_CHAN0               = 0,
	PIT_CMD_CHAN1               = 1 << 6,
	PIT_CMD_CHAN2               = 1 << 7,
	PIT_CMD_READ_BACK           = 3 << 6,

	PIT_CMD_LATCH_COUNT         = 0,
	PIT_CMD_LOBYTE              = 1 << 4,
	PIT_CMD_HIBYTE              = 1 << 5,
	PIT_CMD_HILOBYTE            = 3 << 4,

	PIT_CMD_MODE_TERM_INTERRUPT = 0,
	PIT_CMD_MODE_ONE_SHOT       = 1 << 1,
	PIT_CMD_MODE_RATE_GEN       = 2 << 1,
	PIT_CMD_MODE_SQUARE_WAVE    = 3 << 1,
	PIT_CMD_MODE_SOFT_STROBE    = 4 << 1,
	PIT_CMD_MODE_HARD_STROBE    = 5 << 1,

	PIT_CMD_BINARY              = 0,
	PIT_CMD_BCD                 = 1,
};

enum {
	PIT_READ_LATCH_COUNT  = 1 << 5,
	PIT_READ_LATCH_STATUS = 1 << 4,
	PIT_READ_CHAN2        = 1 << 3,
	PIT_READ_CHAN1        = 1 << 2,
	PIT_READ_CHAN0        = 1 << 1,
	/* bit 0 reserved */
};

enum {
	PIT_STATUS_STATE               = 1 << 7,
	PIT_STATUS_NULL_COUNT          = 1 << 6,

	PIT_STATUS_LATCH_COUNT         = 0,
	PIT_STATUS_LOBYTE              = 1 << 4,
	PIT_STATUS_HIBYTE              = 1 << 5,
	PIT_STATUS_HILOBYTE            = 3 << 4,

	PIT_STATUS_MODE_TERM_INTERRUPT = 0,
	PIT_STATUS_MODE_ONE_SHOT       = 1 << 1,
	PIT_STATUS_MODE_RATE_GEN       = 2 << 1,
	PIT_STATUS_MODE_SQUARE_WAVE    = 3 << 1,
	PIT_STATUS_MODE_SOFT_STROBE    = 4 << 1,
	PIT_STATUS_MODE_HARD_STROBE    = 5 << 1,

	PIT_STATUS_BINARY              = 0,
	PIT_STATUS_BCD                 = 1,
};

enum {
	PIT_WAIT_1MS  = 1193,
	PIT_WAIT_5MS  = 5966,
	PIT_WAIT_10MS = 11932,
	PIT_WAIT_20MS = 23865,
};

void pit_wait_poll( uint16_t value );

#endif
