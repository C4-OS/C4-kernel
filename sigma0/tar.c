#include <sigma0/sigma0.h>
#include <sigma0/tar.h>
#include <stdbool.h>

static inline bool str_equal( const char *a, const char *b ){
	for ( ; *a && *b; a++, b++ ){
		if ( *a != *b ){
			return false;
		}
	}

	return *a == *b;
}

static inline unsigned octal_num( const char *num ){
	unsigned ret = 0;

	for ( ; *num; num++ ){
		ret *= 8;
		ret += *num - '0';
	}

	return ret;
}

tar_header_t *tar_lookup( tar_header_t *archive, const char *name ){
	tar_header_t *ret  = NULL;
	tar_header_t *temp = archive;

	while ( temp && *temp->filename ){
		if ( str_equal( temp->filename, name )){
			ret = temp;
			break;
		}

		unsigned size = octal_num( temp->size );
		temp += size / sizeof( tar_header_t ) + 1;
		temp += size % sizeof( tar_header_t ) > 0;
	}

	return ret;
}

tar_header_t *tar_next( tar_header_t *archive ){
	tar_header_t *ret = archive;

	if ( ret && *ret->filename ){
		unsigned size = octal_num( ret->size );

		ret += size / sizeof( tar_header_t ) + 1;
		ret += size % sizeof( tar_header_t ) > 0;
	}

	return ret;
}

void *tar_data( tar_header_t *archive ){
	return (void *)(archive + 1);
}

unsigned tar_data_size( tar_header_t *archive ){
	return octal_num( archive->size );
}
