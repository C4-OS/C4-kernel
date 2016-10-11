#include <c4/klib/string.h>
#include <stdint.h>

void *memset( void *s, int c, unsigned n ){
	for ( unsigned i = 0; i < n; i++ ){
		((uint8_t *)s)[i] = c;
	}

	return s;
}

unsigned strlen(const char *s){
	unsigned i = 0;

	for ( i = 0; s[i]; i++ );

	return i;
}
