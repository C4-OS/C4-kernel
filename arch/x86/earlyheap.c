// TODO: this can probably be merged into another file in the future
#include <c4/arch/earlyheap.h>
#include <stdint.h>

extern void *end;

void *kealloc( unsigned size ){
	static uint8_t *heap_ptr = (uint8_t *)&end;

	void *ret = heap_ptr;
	heap_ptr += size + (8 - (size % 8));

	return ret;
}
