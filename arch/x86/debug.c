#include <c4/debug.h>
#include <c4/klib/string.h>
#include <stdbool.h>

enum {
	WIDTH  = 80,
	HEIGHT = 25,
};

typedef struct vga_char {
	char text;
	char color;
} __attribute__((packed)) vga_char_t;

typedef struct vga_state {
	vga_char_t *textbuf;
	unsigned x;
	unsigned y;
} vga_state_t;

static void do_scroll( void ){
	vga_char_t *vgatext = (void *)0xc00b8000;

	for ( unsigned y = 0; y < HEIGHT; y++ ){
		memcpy(
			vgatext + WIDTH * y,
			vgatext + WIDTH * (y + 1),
			sizeof( vga_char_t[WIDTH] ));
	}
}

static void do_newline( vga_state_t *state ){
	if ( ++state->y >= HEIGHT - 1 ){
		state->y = HEIGHT - 1;
		do_scroll( );
	}

	state->x = 0;
}

static void plot_char( vga_state_t *state, char c ){
	vga_char_t *vgachar = state->textbuf + (WIDTH * state->y) + state->x;

	vgachar->text = c;
	vgachar->color = 0x7 | 0x10;

	if ( state->x + 1 >= WIDTH ){
		do_newline( state );
	}

	state->x = (state->x + 1) % WIDTH;
}

static void clear_screen( vga_state_t *state ){
	memset( state->textbuf, 0, sizeof( vga_char_t[WIDTH * HEIGHT] ));
}


void debug_putchar( int c ){
	static vga_state_t state = { (vga_char_t *)0xc00b8000, 0, 0 };
	static bool initialized = false;

	if ( !initialized ){
		clear_screen( &state );
		initialized = true;
	}

	switch ( c ){
		case '\n':
			do_newline( &state );
			break;

		case '\r':
			state.x = 0;
			break;

		default:
			plot_char( &state, c );
			break;
	}
}
