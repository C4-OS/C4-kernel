#ifndef _C4_DEBUG_H
#define _C4_DEBUG_H 1
#include <stdbool.h>

#ifndef KNDEBUG
#define KASSERT(CONDITION) { \
		if ( !(CONDITION) ){ \
			debug_printf( "%s:%u: assertion \"" #CONDITION "\" failed\n", \
				__FILE__, __LINE__ ); \
		} \
	}

#else
#define KASSERT(CONDITION) /* CONDITION */
#endif

void debug_putchar( int c );
void debug_puts( const char *str );
void debug_printf( const char *format, ... );

#endif
