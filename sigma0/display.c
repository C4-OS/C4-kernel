#include <sigma0/sigma0.h>
#include <stdint.h>

enum {
	WIDTH  = 80,
	START  = 0,
	HEIGHT = 25,
};

typedef struct vga_char {
	uint8_t text;
	uint8_t color;
} vga_char_t;

typedef struct vga_state {
	vga_char_t *textbuf;
	unsigned x;
	unsigned y;
} vga_state_t;

static inline void scroll_display( vga_state_t *state ){
	for ( unsigned i = START + 1; i < HEIGHT; i++ ){
		vga_char_t *foo = state->textbuf + WIDTH * (i - 1);
		vga_char_t *bar = state->textbuf + WIDTH * i;

		for ( unsigned k = 0; k < WIDTH; k++ ){
			*foo++ = *bar++;
		}

		for ( unsigned k = 0; k < WIDTH; k++ ){
			foo->text = ' ';
			foo++;
		}
	}
}

static inline void do_newline( vga_state_t *state ){
	if ( ++state->y >= HEIGHT - 1 ){
		scroll_display( state );
		state->y = HEIGHT - 1;
	}

	state->x = 0;
}

void display_thread( void *unused ){
	message_t msg;

	vga_state_t state = {
		.textbuf = (void *)0xb8000,
		.x       = 0,
		.y       = START,
	};

	for ( unsigned i = 0; i < WIDTH * HEIGHT; i++ ){
		state.textbuf[i].text = ' ';
		state.textbuf[i].color = 0;
	}

	while ( true ){
		c4_msg_recieve( &msg, 0 );

		char c = msg.data[0];

		if ( c == '\n' ){
			do_newline( &state );

		} else if ( c == '\b' ){
			state.x--;

			vga_char_t *temp = state.textbuf + WIDTH * state.y + state.x;
			temp->text = ' ';

		} else {
			vga_char_t *temp = state.textbuf + WIDTH * state.y + state.x;

			temp->text  = c;
			temp->color = 0x7;

			if ( state.x++ >= WIDTH ){
				do_newline( &state );
			}
		}
	}
}
