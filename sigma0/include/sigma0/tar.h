#ifndef _C4_SIGMA0_TAR_H
#define _C4_SIGMA0_TAR_H
#include <stdint.h>

// this struct doesn't define fields which aren't used,
// such as uid, gid, modification time, permissions, etc.
typedef struct tar_header {
	char filename[100];
	char unused[24];
	char size[12];
	char padding[376];
} tar_header_t;

tar_header_t *tar_lookup( tar_header_t *archive, const char *name );
void         *tar_data( tar_header_t *archive );
unsigned      tar_data_size( tar_header_t *archive );

#endif
