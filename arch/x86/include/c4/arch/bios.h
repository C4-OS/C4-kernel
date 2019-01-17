#ifndef _MP_BIOS_H
#define _MP_BIOS_H 1
#include <stddef.h>

enum {
	BIOS_RESET_VECTOR = 0x467,
};

void *find_bios_key( const char *s, void *mem, size_t n );

#endif
