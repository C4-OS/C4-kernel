#include <c4/klib/string.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *to = dest;
	const uint8_t *from = src;

	for (size_t i = 0; i < n; i++) {
		to[i] = from[i];
	}

	return dest;
}

void *memset(void *s, int c, size_t n) {
	for (size_t i = 0; i < n; i++) {
		((uint8_t *)s)[i] = c;
	}

	return s;
}

size_t strlen(const char *s) {
	size_t i = 0;

	for (i = 0; s[i]; i++);

	return i;
}

char *strncpy(char *dest, const char *src, size_t n) {
	size_t i = 0;

	for (; i < n - 1 && src[i]; i++) {
		dest[i] = src[i];
	}

	dest[i] = 0;

	return dest;
}
