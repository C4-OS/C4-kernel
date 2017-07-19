#include <c4/syscall.h>
#include <c4/debug.h>
#include <c4/message.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/mm/addrspace.h>
#include <c4/capability.h>
#include <c4/error.h>
#include <stddef.h>

#define SYSCALL_ARGS arg_t a, arg_t b, arg_t c, arg_t d

typedef uintptr_t arg_t;
typedef int (*syscall_func_t)( SYSCALL_ARGS );

static int syscall_thread_exit( SYSCALL_ARGS );
static int syscall_thread_create( SYSCALL_ARGS );
static int syscall_thread_set_addrspace( SYSCALL_ARGS );
static int syscall_thread_set_capspace( SYSCALL_ARGS );
static int syscall_thread_set_stack( SYSCALL_ARGS );
static int syscall_thread_set_ip( SYSCALL_ARGS );
static int syscall_thread_set_priority( SYSCALL_ARGS );

static int syscall_sync_create( SYSCALL_ARGS );
static int syscall_sync_send( SYSCALL_ARGS );
static int syscall_sync_recieve( SYSCALL_ARGS );

static int syscall_async_create( SYSCALL_ARGS );
static int syscall_async_send( SYSCALL_ARGS );
static int syscall_async_recieve( SYSCALL_ARGS );

static int syscall_addrspace_create( SYSCALL_ARGS );
static int syscall_addrspace_map( SYSCALL_ARGS );
static int syscall_addrspace_unmap( SYSCALL_ARGS );

static int syscall_phys_frame_split( SYSCALL_ARGS );
static int syscall_phys_frame_join( SYSCALL_ARGS );

static int syscall_cspace_create( SYSCALL_ARGS );
static int syscall_cspace_cap_move( SYSCALL_ARGS );
static int syscall_cspace_cap_remove( SYSCALL_ARGS );
static int syscall_cspace_cap_restrict( SYSCALL_ARGS );
static int syscall_cspace_cap_share( SYSCALL_ARGS );
static int syscall_cspace_cap_grant( SYSCALL_ARGS );

// TODO: look into replacing this syscall with io bitmap
static int syscall_ioport( SYSCALL_ARGS );
static int syscall_info( SYSCALL_ARGS );
static int syscall_debug_putchar( SYSCALL_ARGS );

static const syscall_func_t syscall_table[SYSCALL_MAX] = {
	// thread syscalls
	syscall_thread_exit,
	syscall_thread_create,
	syscall_thread_set_addrspace,
	syscall_thread_set_capspace,
	syscall_thread_set_stack,
	syscall_thread_set_ip,
	syscall_thread_set_priority,

	// syncronous ipc syscalls
	syscall_sync_create,
	syscall_sync_send,
	syscall_sync_recieve,

	// asyncronous ipc syscalls
	syscall_async_create,
	syscall_async_send,
	syscall_async_recieve,

	// address space syscalls
	syscall_addrspace_create,
	syscall_addrspace_map,
	syscall_addrspace_unmap,

	// physical memory frame syscalls
	syscall_phys_frame_split,
	syscall_phys_frame_join,

	// capability space management syscalls
	syscall_cspace_create,
	syscall_cspace_cap_move,
	syscall_cspace_cap_remove,
	syscall_cspace_cap_restrict,
	syscall_cspace_cap_share,
	syscall_cspace_cap_grant,

	// other syscalls
	syscall_ioport,
	syscall_info,
	syscall_debug_putchar,
};

int syscall_dispatch( unsigned num, arg_t a, arg_t b, arg_t c, arg_t d ){
	if ( num >= SYSCALL_MAX ){
		return -1;
	}

	return syscall_table[num](a, b, c, d);
}

// TODO: consider moving this to capability.c
static int do_cap_check( thread_t *thread,
                         cap_entry_t **entry,
						 uint32_t object,
                         uint32_t type,
                         uint32_t permissions )
{
	if ( !thread->cap_space ){
		debug_printf( "- invalid cap space! %u\n", thread->id );
		return -C4_ERROR_PERMISSION_DENIED;
	}

	cap_entry_t *cap = cap_space_lookup( thread->cap_space, object );

	if ( !cap || cap->type != CAP_TYPE_IPC_SYNC_ENDPOINT ){
		debug_printf( "- invalid cap/cap type! %u\n", thread->id );
		return -C4_ERROR_INVALID_OBJECT;
	}

	if ( (cap->permissions & permissions) != permissions ){
		debug_printf( "- invalid permissions! %u\n", thread->id );
		return -C4_ERROR_PERMISSION_DENIED;
	}

    *entry = cap;
    return C4_ERROR_NONE;
}


static int syscall_thread_exit( arg_t a, arg_t b, arg_t c, arg_t d ){
	thread_t *cur = sched_current_thread( );

	debug_printf( "thread %u exiting\n", cur->id );
	sched_thread_exit( );

	// shouldn't get here
	debug_printf( "sched_thread_exit() failed for some reason...?\n" );
	return 0;
}

static int syscall_thread_create( arg_t user_entry,
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

		return -C4_ERROR_INVALID_ARGUMENT;
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

	// TODO: return capability space address rather than thread id
	return thread->id;
}

static int syscall_thread_set_addrspace( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_thread_set_capspace( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_thread_set_stack( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_thread_set_ip( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_thread_set_priority( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_sync_create( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_sync_send( arg_t buffer, arg_t target, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	cap_entry_t *cap = NULL;

	int check = do_cap_check( sched_current_thread(), &cap, target,
	                          CAP_TYPE_IPC_SYNC_ENDPOINT, CAP_MODIFY );

	if ( check != C4_ERROR_NONE ){
		return check;
	}

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		//return -1;
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	//message_send( msg, target );
	message_send( cap->object, msg );

	return 0;
}

static int syscall_sync_recieve( arg_t buffer, arg_t from, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	cap_entry_t *cap = NULL;

	int check = do_cap_check( sched_current_thread(), &cap, from,
	                          CAP_TYPE_IPC_SYNC_ENDPOINT, CAP_ACCESS );

	if ( check != C4_ERROR_NONE ){
		return check;
	}

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	//message_recieve( msg, from );
	message_recieve( cap->object, msg );

	return 0;
}

static int syscall_async_create( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_async_send( arg_t buffer, arg_t to, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	cap_entry_t *cap = NULL;

	int check = do_cap_check( sched_current_thread(), &cap, to,
	                          CAP_TYPE_IPC_ASYNC_ENDPOINT, CAP_MODIFY );

	if ( check != C4_ERROR_NONE ){
		return check;
	}

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	//return message_send_async( msg, to );
	return message_send_async( cap->object, msg );
}

static int syscall_async_recieve( arg_t buffer,
                                  arg_t from,
                                  arg_t flags,
                                  arg_t d )
{
	message_t *msg = (message_t *)buffer;
	cap_entry_t *cap = NULL;

	int check = do_cap_check( sched_current_thread(), &cap, from,
	                          CAP_TYPE_IPC_ASYNC_ENDPOINT, CAP_ACCESS );

	if ( check != C4_ERROR_NONE ){
		return check;
	}

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );

		return false;
	}

	//return message_recieve_async( msg, flags );
	return message_recieve_async( cap->object, msg, flags );
}

static int syscall_addrspace_create( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_addrspace_map( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_addrspace_unmap( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_phys_frame_split( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_phys_frame_join( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_create( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_move( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_remove( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_restrict( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_share( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_grant( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
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

			/*
		case SYSCALL_INFO_GET_PAGER:
			return current->pager;
			break;
			*/

		default:
			break;
	}

	return -1;
}

static int syscall_debug_putchar( arg_t character, arg_t b, arg_t c, arg_t d ){
	debug_putchar( character );

	return 0;
}
