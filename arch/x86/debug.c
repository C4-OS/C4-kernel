#include <c4/debug.h>

typedef struct vga_char {
	char text;
	char color;
} __attribute__((packed)) vga_char_t;

static unsigned x_pos = 0;
static unsigned y_pos = 0;

void debug_putchar( int c ){
	vga_char_t *vgatext = (void *)0xc00b8000;

	switch ( c ){
		case '\n':
			y_pos = (y_pos + 1) % 25;
			x_pos = 0;
			break;

		case '\r':
			x_pos = 0;
			break;

		default:
			vgatext[(80 * y_pos) + x_pos].text = c;
			vgatext[(80 * y_pos) + x_pos].color = 0x7 | 0x10;

			if ( x_pos + 1 > 80 ){
				y_pos = (y_pos + 1) % 25;
			}

			x_pos = (x_pos + 1) % 80;
			break;
	}
}
