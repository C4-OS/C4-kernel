#include <c4/klib/string.h>
#include <c4/arch/paging.h>
#include <c4/debug.h>
#include <stdbool.h>

enum {
	WIDTH  = 80,
	HEIGHT = 15,
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
	vga_char_t *vgatext = (void *)low_phys_to_virt( 0xb8000 );

	for ( unsigned y = 1; y < HEIGHT; y++ ){
		memcpy(
			vgatext + WIDTH * (y - 1),
			vgatext + WIDTH * y,
			sizeof( vga_char_t[WIDTH] ));

		memset( vgatext + WIDTH * y, 0, sizeof( vga_char_t[WIDTH] ));
	}

	for ( unsigned x = 0; x < WIDTH; x++ ){
		vga_char_t *c = vgatext + x + WIDTH * (HEIGHT);

		c->text = '=';
		c->color = 0x7;
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
	static vga_state_t state;
	static bool initialized = false;
	static bool pending_newline = false;

	if ( !initialized ){
		state = (vga_state_t){
			.textbuf = (vga_char_t *)low_phys_to_virt( 0xb8000 ),
			.x       = 0,
			.y       = 0
		};

		clear_screen( &state );

		initialized = true;
	}

	if ( pending_newline ){
		do_newline( &state );
		pending_newline = false;
	}

	switch ( c ){
		case '\n':
			//do_newline( &state );
			pending_newline = true;
			break;

		case '\r':
			state.x = 0;
			break;

		default:
			plot_char( &state, c );
			break;
	}
}
