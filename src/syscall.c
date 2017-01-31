#include <c4/syscall.h>
#include <c4/debug.h>
#include <c4/message.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/mm/addrspace.h>

typedef uintptr_t arg_t;
typedef int (*syscall_func_t)( arg_t a, arg_t b, arg_t c, arg_t d );

static int syscall_exit( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_create_thread( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_send( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_recieve( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_send_async( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_recieve_async( arg_t a, arg_t b, arg_t c, arg_t d );
// TODO: look into replacing this syscall with io bitmap
static int syscall_ioport( arg_t a, arg_t b, arg_t c, arg_t d );
static int syscall_info( arg_t a, arg_t b, arg_t c, arg_t d );

static const syscall_func_t syscall_table[SYSCALL_MAX] = {
	syscall_exit,
	syscall_create_thread,
	syscall_send,
	syscall_recieve,
	syscall_send_async,
	syscall_recieve_async,
	syscall_ioport,
	syscall_info,
};

int syscall_dispatch( unsigned num, arg_t a, arg_t b, arg_t c, arg_t d ){
	if ( num >= SYSCALL_MAX ){
		return -1;
	}

	return syscall_table[num](a, b, c, d);
}

static int syscall_exit( arg_t a, arg_t b, arg_t c, arg_t d ){
	thread_t *cur = sched_current_thread( );

	debug_printf( "thread %u exiting\n", cur->id );
	sched_thread_exit( );

	// shouldn't get here
	debug_printf( "sched_thread_exit() failed for some reason...?\n" );
	return 0;
}

static int syscall_create_thread( arg_t user_entry,
                                  arg_t user_stack,
                                  arg_t flags,
                                  arg_t d )
{
	void (*entry)(void) = (void *)user_entry;
	void *stack         = (void *)user_stack;

	thread_t *thread;
	thread_t *cur = sched_current_thread( );

	if ( !is_user_address( entry ) || !is_user_address( stack ))
	{
		debug_printf( "%s: invalid argument, entry: %p, stack: %p\n",
		              entry, stack );

		return -1;
	}

	addr_space_t *space = cur->addr_space;

	if ( flags & THREAD_CREATE_FLAG_CLONE ){
		space = addr_space_clone( space );

	} else if ( flags & THREAD_CREATE_FLAG_NEWMAP ){
		space = addr_space_clone( addr_space_kernel( ));
		addr_space_set( space );
		addr_space_map_self( space, ADDR_MAP_ADDR );
		addr_space_set( cur->addr_space );
	}

	thread = thread_create( entry, space, stack, THREAD_FLAG_USER );

	sched_thread_stop( thread );
	sched_add_thread( thread );

	debug_printf( ">> created user thread %u\n", thread->id );
	debug_printf( ">>      entry: %p\n", thread->registers.eip );
	debug_printf( ">>      stack: %p\n", thread->registers.esp );
	debug_printf( ">>    current: %p\n", thread );

	return thread->id;
}

static int syscall_send( arg_t buffer, arg_t target, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	//unsigned id = sched_current_thread()->id;

	//debug_printf( "%u: trying to send message %p to %u\n", id, msg, target );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -1;
	}

	message_send( msg, target );

	return 0;
}

static int syscall_recieve( arg_t buffer, arg_t from, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	//unsigned id = sched_current_thread()->id;

	//debug_printf( "%u: trying to recieve message at %p\n", id, msg );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -1;
	}

	message_recieve( msg, from );

	return 0;
}

static int syscall_send_async( arg_t buffer, arg_t to, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );

		return false;
	}

	return message_send_async( msg, to );
}

static int syscall_recieve_async( arg_t buffer, arg_t flags, arg_t c, arg_t d )
{
	message_t *msg = (message_t *)buffer;

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );

		return false;
	}

	return message_recieve_async( msg, flags );
}


#ifdef __i386__
#include <c4/arch/ioports.h>
#endif

static int syscall_ioport( arg_t action, arg_t port, arg_t value, arg_t d ){
#ifdef __i386__
	switch ( action ){
		case IO_PORT_IN_BYTE:  return inb( port );
		case IO_PORT_IN_WORD:  return inw( port );
		case IO_PORT_IN_DWORD: return inl( port );

		case IO_PORT_OUT_BYTE:  outb( port, value ); return 0;
		case IO_PORT_OUT_WORD:  outw( port, value ); return 0;
		case IO_PORT_OUT_DWORD: outl( port, value ); return 0;

		default: break;
	}
#endif

	// TODO: return proper error
	return -1;
}

static int syscall_info( arg_t action, arg_t b, arg_t c, arg_t d ){
	thread_t *current = sched_current_thread();

	switch ( action ){
		case SYSCALL_INFO_GET_ID:
			return current->id;
			break;

		case SYSCALL_INFO_GET_PAGER:
			return current->pager;
			break;

		default:
			break;
	}

	return -1;
}
