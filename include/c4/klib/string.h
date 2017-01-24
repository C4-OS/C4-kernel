#ifndef _C4_KLIB_STRING_H
#define _C4_KLIB_STRING_H 1

void *memcpy( void *dest, const void *src, unsigned n );
void *memset(void *s, int c, unsigned n);
unsigned strlen(const char *s);
char *strncpy( char *dest, const char *src, unsigned n );

#endif
