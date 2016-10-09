#include <c4/arch/segments.h>

typedef struct vga_char {
	char text;
	char color;
} __attribute__((packed)) vga_char_t;

void arch_init( void ){
	vga_char_t *foo = (void *)0xc00b8000;
	char *s = " hello, world! ";

	init_segment_descs( );

	for ( unsigned i = 0; s[i]; i++ ){
		foo[i + 3].text  = s[i];
		foo[i + 3].color = 07;
	}

	while ( 1 ){
		s = "/-\\|";

		for ( unsigned i = 0; s[i]; i++ ){
			foo[0].text = '[';
			foo[1].text = s[i];
			foo[2].text = ']';

			foo[0].color = 07;
			foo[1].color = 02;
			foo[2].color = 07;

			for ( volatile unsigned j = 30000000; j; j-- );
		}
	}
}
