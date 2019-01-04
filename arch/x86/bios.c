#include <c4/arch/mp/bios.h>
#include <stddef.h>
#include <stdbool.h>

bool asdf( const char *a, const char *b, size_t size ){
	for ( size_t n = 0; n < size; n++ ){
		if ( a[n] != b[n] ){
			return false;
		}
	}

	return true;
}

void *find_bios_key( const char *s, void *mem, size_t n ){
	char *ptr = mem;

	for ( size_t i = 0; i + 3 < n; i++ ){
		if ( asdf( s, ptr + i, 4 )){
			return ptr + i;
		}
	}

	return NULL;
}
