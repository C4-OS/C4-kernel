#ifndef _C4_DEBUG_H
#define _C4_DEBUG_H 1
#include <stdbool.h>

void debug_putchar( int c );
void debug_puts( const char *str );
void debug_printf( const char *format, ... );

#endif
