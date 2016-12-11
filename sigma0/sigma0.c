#include <sigma0/sigma0.h>
#include <sigma0/tar.h>
#include <sigma0/elf.h>
#include <miniforth/miniforth.h>
#include <c4/thread.h>

struct foo {
	int target;
	int display;
	int forth;
};

// external binaries linked into the image
// forth initial commands file
extern char _binary_sigma0_init_commands_fs_start[];
extern char _binary_sigma0_init_commands_fs_end[];
// tar
extern char _binary_sigma0_initfs_tar_start[];
extern char _binary_sigma0_initfs_tar_end[];

static tar_header_t *tar_initfs = (void *)_binary_sigma0_initfs_tar_start;

void test_thread( void *unused );
void forth_thread( void *sysinfo );
void debug_print( struct foo *info, char *asdf );
int  elf_load( Elf32_Ehdr *elf, int display );

static void *allot_pages( unsigned pages );
static void *allot_stack( unsigned pages );
static void *stack_push( unsigned *stack, unsigned foo );

void main( void ){
	struct foo thing;

	unsigned *s = allot_stack( 1 );
	s -= 1;
	thing.display = c4_create_thread( display_thread, s, 0 );

	s = allot_stack( 2 );
	s = stack_push( s, (unsigned)&thing );

	thing.forth = c4_create_thread( forth_thread, s, THREAD_CREATE_FLAG_CLONE);

	c4_continue_thread( thing.display );

	tar_header_t *arc = tar_initfs;
	tar_header_t *test = tar_lookup( arc, "sigma0/initfs/bin/test" );

	if ( test ){
		debug_print( &thing, "tar: found test file, loading...\n" );

		uint8_t  *data = tar_data( test );
		unsigned  size = tar_data_size( test );

		elf_load( (void *)data, 2 );

	} else {
		debug_print( &thing, "didn't find test...\n" );
	}

	c4_mem_map_to( thing.forth, allot_pages(1), (void *)0x12345000, 1,
	               PAGE_READ | PAGE_WRITE );

	c4_continue_thread( thing.forth );

	server( &thing );

	// TODO: panic or dump debug info or something, server()
	//       should never return
	for ( ;; );
}

static void *allot_pages( unsigned pages ){
	static uint8_t *s = (void *)0xd0001000;
	void *ret = s;

	s += pages * PAGE_SIZE;

	return ret;
}

static void *allot_stack( unsigned pages ){
	uint8_t *ret = allot_pages( pages );

	ret += pages * PAGE_SIZE - 4;

	return ret;
}

static void *stack_push( unsigned *stack, unsigned foo ){
	*(stack--) = foo;

	return stack;
}

int elf_load( Elf32_Ehdr *elf, int display ){
	unsigned stack_offset = 0xff8;

	void *entry      = (void *)elf->e_entry;
	void *to_stack   = (uint8_t *)0xa0000000;
	void *from_stack = (uint8_t *)allot_pages(1);
	void *stack      = (uint8_t *)to_stack + stack_offset;

	// copy the output pointer to the new stack
	*(unsigned *)((uint8_t *)from_stack + stack_offset + 4) = (unsigned)display;

	int thread_id = c4_create_thread( entry, stack,
	                                  THREAD_CREATE_FLAG_NEWMAP);

	c4_mem_grant_to( thread_id, from_stack, to_stack, 1,
	                 PAGE_READ | PAGE_WRITE );

	// load program headers
	for ( unsigned i = 0; i < elf->e_phnum; i++ ){
		Elf32_Phdr *header = elf_get_phdr( elf, i );
		uint8_t *progdata  = (uint8_t *)((uintptr_t)elf + header->p_offset );
		void    *addr      = (void *)header->p_vaddr;
		unsigned pages     = header->p_memsz / PAGE_SIZE
		                   + header->p_memsz % PAGE_SIZE > 0;
		uint8_t *databuf   = allot_pages( pages );

		for ( unsigned k = 0; k < header->p_filesz; k++ ){
			databuf[k] = progdata[k];
		}

		// TODO: translate elf permissions into message permissions
		c4_mem_grant_to( thread_id, databuf, addr, pages,
		                 PAGE_READ | PAGE_WRITE );
	}

	c4_continue_thread( thread_id );

	return 0;
}

void test_thread( void *data ){
	message_t msg;
	volatile struct foo *meh = data;

	msg.type = 0xcafe;

	for (;;){
		c4_msg_recieve( &msg, 0 );
		c4_msg_send( &msg, meh->target );
	}
}

extern const char *foo;

void server( void *data ){
	message_t msg;
	struct foo *meh = data;

	while ( true ){
		c4_msg_recieve( &msg, 0 );

		char c = decode_scancode( msg.data[0] );

		if ( c && msg.data[1] == 0 ){
			if ( c ){
				message_t keycode;
				keycode.type    = 0xbabe;
				keycode.data[0] = c;

				c4_msg_send( &keycode, meh->display );
				c4_msg_send( &keycode, meh->forth );
			}
		}
	}

	for ( ;; );
}

enum {
	CODE_ESCAPE,
	CODE_TAB,
	CODE_LEFT_CONTROL,
	CODE_RIGHT_CONTROL,
	CODE_LEFT_SHIFT,
	CODE_RIGHT_SHIFT,
};

const char lowercase[] =
	{ '`', CODE_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',
	  '=', '\b', CODE_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	  '[', ']', '\n', CODE_LEFT_CONTROL, 'a', 's', 'd', 'f', 'g', 'h', 'j',
	  'k', 'l', ';', '\'', '?', CODE_LEFT_SHIFT, '?', 'z', 'x', 'c', 'v', 'b',
	  'n', 'm', ',', '.', '/', CODE_RIGHT_SHIFT, '_', '_', ' ', '_', '_', '_',
	  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
	};

const char uppercase[] =
	{ '~', CODE_ESCAPE, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
	  '+', '\b', CODE_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	  '{', '}', '\n', CODE_LEFT_CONTROL, 'A', 'S', 'D', 'F', 'G', 'H', 'J',
	  'K', 'L', ':', '"', '?', CODE_LEFT_SHIFT, '?', 'Z', 'X', 'C', 'V', 'B',
	  'N', 'M', '<', '>', '?', CODE_RIGHT_SHIFT, '_', '_', ' ', '_', '_', '_',
	  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
	};

char decode_scancode( unsigned long code ){
	static bool is_uppercase = false;
	char c = is_uppercase? uppercase[code] : lowercase[code];
	char ret = '\0';

	switch ( c ){
		case CODE_LEFT_SHIFT:
		case CODE_RIGHT_SHIFT:
			is_uppercase = !is_uppercase;
			break;

		default:
			ret = c;
			break;
	}

	return ret;
}

static struct foo *forth_sysinfo;

static char *read_line( char *buf, unsigned n ){
	message_t msg;
	unsigned i = 0;

	for ( i = 0; i < n - 1; i++ ){
retry:
		c4_msg_recieve( &msg, 0 );
		char c = msg.data[0];

		if ( i && c == '\b' ){
			i--;
			goto retry;
		}

		buf[i] = c;

		if ( c == '\n' ){
			break;
		}
	}

	buf[++i] = '\0';

	return buf;
}

char minift_get_char( void ){
	static char input[80];
	static bool initialized = false;
	static char *ptr;

	if ( !initialized ){
		*(_binary_sigma0_init_commands_fs_end - 1) = 0;

		for ( unsigned i = 0; i < sizeof(input); i++ ){ input[i] = 0; }
		ptr         = _binary_sigma0_init_commands_fs_start;
		initialized = true;
	}

	while ( !*ptr ){
		debug_print( forth_sysinfo, "miniforth > " );
		ptr = read_line( input, sizeof( input ));
	}

	return *ptr++;
}

void minift_put_char( char c ){
	message_t msg;

	msg.type    = 0xbabe;
	msg.data[0] = c;

	c4_msg_send( &msg, forth_sysinfo->display );
}

static bool c4_minift_sendmsg( minift_vm_t *vm );
static bool c4_minift_recvmsg( minift_vm_t *vm );
static bool c4_minift_tarfind( minift_vm_t *vm );
static bool c4_minift_tarsize( minift_vm_t *vm );
static bool c4_minift_tarnext( minift_vm_t *vm );

static minift_archive_entry_t c4_words[] = {
	{ "sendmsg", c4_minift_sendmsg, 0 },
	{ "recvmsg", c4_minift_recvmsg, 0 },
	{ "tarfind", c4_minift_tarfind, 0 },
	{ "tarsize", c4_minift_tarsize, 0 },
	{ "tarnext", c4_minift_tarnext, 0 },
};

void forth_thread( void *sysinfo ){
	forth_sysinfo = sysinfo;

	volatile unsigned x = forth_sysinfo->display;

	unsigned long data[1024 + 512];
	unsigned long calls[32];
	unsigned long params[32];

	minift_vm_t foo;
	minift_archive_t arc = {
		.name    = "c4",
		.entries = c4_words,
		.size    = sizeof(c4_words) / sizeof(minift_archive_entry_t),
	};

	for ( ;; ){
		minift_stack_t data_stack = {
			.start = data,
			.ptr   = data,
			.end   = data + 1536,
		};

		minift_stack_t call_stack = {
			.start = calls,
			.ptr   = calls,
			.end   = calls + 32,
		};

		minift_stack_t param_stack = {
			.start = params,
			.ptr   = params,
			.end   = params + 32,
		};

		minift_init_vm( &foo, &call_stack, &data_stack, &param_stack, NULL );
		minift_archive_add( &foo, &arc );
		minift_run( &foo );
		debug_print( sysinfo, "forth vm exited, restarting...\n" );
	}
}

void debug_print( struct foo *info, char *str ){
	message_t msg;

	for ( unsigned i = 0; str[i]; i++ ){
		msg.data[0] = str[i];
		msg.type    = 0xbabe;

		c4_msg_send( &msg, info->display );
	}
}

int c4_msg_send( message_t *buffer, unsigned to ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_SEND, buffer, to, 0, 0, ret );

	return ret;
}

int c4_msg_recieve( message_t *buffer, unsigned from ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_RECIEVE, buffer, from, 0, 0, ret );

	return ret;
}

int c4_create_thread( void (*entry)(void *), void *stack, unsigned flags ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_CREATE_THREAD, entry, stack, flags, 0, ret );

	return ret;
}

int c4_continue_thread( unsigned thread ){
	message_t buf = { .type = MESSAGE_TYPE_CONTINUE, };

	return c4_msg_send( &buf, thread );
}

void *c4_request_physical( uintptr_t virt,
                           uintptr_t physical,
                           unsigned size,
                           unsigned permissions )
{
	message_t msg = {
		.type = MESSAGE_TYPE_REQUEST_PHYS,
		.data = {
			virt,
			physical,
			size,
			permissions
		},
	};

	c4_msg_send( &msg, 0 );

	return (void *)virt;
}

int c4_mem_map_to( unsigned thread_id,
                   void *from,
                   void *to,
                   unsigned size,
                   unsigned permissions )
{
	message_t msg = {
		.type = MESSAGE_TYPE_MAP_TO,
		.data = {
			(uintptr_t)from,
			(uintptr_t)to,
			(uintptr_t)size,
			(uintptr_t)permissions,
		},
	};

	return c4_msg_send( &msg, thread_id );
}

int c4_mem_grant_to( unsigned thread_id,
                     void *from,
                     void *to,
                     unsigned size,
                     unsigned permissions )
{
	message_t msg = {
		.type = MESSAGE_TYPE_GRANT_TO,
		.data = {
			(uintptr_t)from,
			(uintptr_t)to,
			(uintptr_t)size,
			(uintptr_t)permissions,
		},
	};

	return c4_msg_send( &msg, thread_id );
}


static bool c4_minift_sendmsg( minift_vm_t *vm ){
	unsigned long target = minift_pop( vm, &vm->param_stack );
	unsigned long temp   = minift_pop( vm, &vm->param_stack );
	message_t *msg = (void *)temp;

	if ( !vm->running ){
		return false;
	}

	debug_print( forth_sysinfo, "got to sendmsg\n" );
	c4_msg_send( msg, target );

	return true;
}

static bool c4_minift_recvmsg( minift_vm_t *vm ){
	//  TODO: add 'from' argument, once that's supported
	unsigned long temp   = minift_pop( vm, &vm->param_stack );
	message_t *msg = (void *)temp;

	if ( !vm->running ){
		return false;
	}

	debug_print( forth_sysinfo, "got to recvmsg\n" );
	c4_msg_recieve( msg, 0 );

	return true;
}

static bool c4_minift_tarfind( minift_vm_t *vm ){
	const char   *name   = (const char *)minift_pop( vm, &vm->param_stack );

	// simulate a "root" directory, which just returns the start of the
	// tar archive
	if ( name[0] == '/' && name[1] == '\0' ){
		minift_push( vm, &vm->param_stack, (unsigned long)tar_initfs );

	} else {
		tar_header_t *lookup = tar_lookup( tar_initfs, name );
		minift_push( vm, &vm->param_stack, (unsigned long)lookup );
	}

	return true;
}

static bool c4_minift_tarsize( minift_vm_t *vm ){
	tar_header_t *lookup = (tar_header_t *)minift_pop( vm, &vm->param_stack );

	if ( lookup ){
		minift_push( vm, &vm->param_stack, tar_data_size( lookup ));

	} else {
		minift_push( vm, &vm->param_stack, 0 );
	}

	return true;
}

static bool c4_minift_tarnext( minift_vm_t *vm ){
	tar_header_t *temp = (tar_header_t *)minift_pop( vm, &vm->param_stack );

	temp = tar_next( temp );
	minift_push( vm, &vm->param_stack, (unsigned long)temp );

	return true;
}
