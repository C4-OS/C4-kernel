#include <c4/klib/string.h>
#include <stdint.h>

void *memset( void *s, int c, unsigned n ){
	for ( unsigned i = 0; i < n; i++ ){
		((uint8_t *)s)[i] = c;
	}

	return s;
}
