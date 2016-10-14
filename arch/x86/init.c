#include <c4/arch/segments.h>
#include <c4/arch/interrupts.h>
#include <c4/arch/ioports.h>
#include <c4/arch/pic.h>
//#include <c4/arch/paging.h>
#include <c4/paging.h>
#include <c4/debug.h>

void arch_init( void ){
	debug_puts( ">> Booting C4 kernel\n" );
	debug_puts( "Initializing gdt... " );
	init_segment_descs( );
	debug_puts( "done\n" );

	debug_puts( "Initializing pic... " );
	remap_pic_vectors_default( );
	debug_puts( "done\n" );

	debug_puts( "Initializing interrupts... " );
	//remap_pic_vectors( 0x20, 0x28 );
	init_interrupts( );
	debug_puts( "done\n" );

	debug_puts( "Initializing more paging structures... ");
	init_paging( );
	debug_puts( "done\n" );

	//asm volatile ( "mov $0xfd, %al; outb $0x21; ");
	//asm volatile ( "mov $0xff, %al; outb $0xa1; ");
	//outb( 0xa1, 0xff );
	//outb( 0x21, 0xfd );

	asm volatile ( "sti" );
	asm volatile ( "int $1" );
	//asm volatile ( "int $3" );
	//asm volatile ( "int $5" );
	//asm volatile ( "int $15" );

	//volatile int a = 0;
	//asm volatile ("cli");

	//for ( volatile unsigned i = 0; i < 0xffffffff; i++ ){
		//debug_puts( "a" );
	//}
	//while ( 1 );

	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );
	//map_page( PAGE_READ | PAGE_WRITE, (void*)0xc0000000 );

	*(uint8_t *)(0xa0000000) = 123;
	debug_printf( "testing: %x\n", *(page_dir_t *)(0xfffff000));

	unmap_page( (void*)0xa0000000 );
	unmap_page( (void*)0xa0000000 );
	map_page( PAGE_READ | PAGE_WRITE, (void*)0xa0000000 );
	*(uint8_t *)(0xa0000000) = 123;

	debug_puts( "hello, world!\n" );

	for ( ;; );
}
