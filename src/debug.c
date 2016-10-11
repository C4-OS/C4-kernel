#include <c4/debug.h>
#include <c4/klib/string.h>
#include <stdarg.h>
#include <stdint.h>

const char *hexadecimal = "0123456789abcdef";
const char *decimal     = "0123456789";
const char *binary      = "01";

void debug_puts( const char *str ){
	for ( unsigned i = 0; str[i]; i++ ){
		debug_putchar( str[i] );
	}
}

void debug_print_num( unsigned long n, const char *base_str ){
	char stack[33];
	unsigned base = strlen(base_str);
	unsigned i;

	if ( n == 0 || base == 0 ){
		debug_putchar( '0' );
		return;
	}

	for ( i = 0; n; i++ ){
		stack[i] = base_str[n % base];
		n /= base;
	}

	while ( i ){
		debug_putchar( stack[--i] );
	}
}

void debug_printf( const char *format, ... ){
	va_list args;
	va_start( args, format );

	for ( unsigned i = 0; format[i]; i++ ){
		if ( format[i] == '%' ){
			switch( format[++i] ){
				case 's':
					debug_puts( va_arg( args, char* ));
					break;

				case 'u':
					debug_print_num( va_arg( args, unsigned ), decimal );
					break;

				case 'x':
					debug_print_num( va_arg( args, unsigned ), hexadecimal );
					break;

				case 'b':
					debug_print_num( va_arg( args, unsigned ), binary );
					break;

				case 'p':
					debug_puts( "0x" );
					debug_print_num( va_arg( args, uintptr_t ), hexadecimal );
					break;

				default:
					break;
			}

		} else {
			debug_putchar( format[i] );
		}
	}
	
	va_end( args );
}
