#include <c4/syscall.h>
#include <c4/debug.h>
#include <c4/message.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/mm/addrspace.h>
#include <c4/capability.h>
// for interrupt_listen()
#include <c4/interrupts.h>

#include <c4/error.h>
#include <stddef.h>

#define SYSCALL_ARGS arg_t a, arg_t b, arg_t c, arg_t d

#define CAP_CHECK_INIT() \
	__attribute__((unused)) cap_entry_t *cap_ent = NULL; \
	__attribute__((unused)) int cap_check = 0; \
	__attribute__((unused)) void *cap_obj = NULL;

#define CAP_CHECK(PTR, X, TYPE, PERMS) \
	{ \
		thread_t *cur = sched_current_thread(); \
		cap_obj = NULL; \
		cap_check = do_cap_check( cur, &cap_ent, (X), (TYPE), (PERMS) ); \
		if ( cap_check != C4_ERROR_NONE ){ \
			return cap_check; \
		} \
		cap_obj = cap_ent->object; \
		(PTR) = cap_ent->object; \
	}

typedef uintptr_t arg_t;
typedef int (*syscall_func_t)( SYSCALL_ARGS );

static int syscall_thread_exit( SYSCALL_ARGS );
static int syscall_thread_create( SYSCALL_ARGS );
static int syscall_thread_set_addrspace( SYSCALL_ARGS );
static int syscall_thread_set_capspace( SYSCALL_ARGS );
static int syscall_thread_set_stack( SYSCALL_ARGS );
static int syscall_thread_set_ip( SYSCALL_ARGS );
static int syscall_thread_set_priority( SYSCALL_ARGS );
static int syscall_thread_set_pager( SYSCALL_ARGS );
static int syscall_thread_continue( SYSCALL_ARGS );
static int syscall_thread_stop( SYSCALL_ARGS );

static int syscall_sync_create( SYSCALL_ARGS );
static int syscall_sync_send( SYSCALL_ARGS );
static int syscall_sync_recieve( SYSCALL_ARGS );

static int syscall_async_create( SYSCALL_ARGS );
static int syscall_async_send( SYSCALL_ARGS );
static int syscall_async_recieve( SYSCALL_ARGS );

static int syscall_addrspace_create( SYSCALL_ARGS );
static int syscall_addrspace_map( SYSCALL_ARGS );
static int syscall_addrspace_unmap( SYSCALL_ARGS );

static int syscall_phys_frame_create( SYSCALL_ARGS );
static int syscall_phys_frame_split( SYSCALL_ARGS );
static int syscall_phys_frame_join( SYSCALL_ARGS );

static int syscall_cspace_create( SYSCALL_ARGS );
static int syscall_cspace_cap_move( SYSCALL_ARGS );
static int syscall_cspace_cap_copy( SYSCALL_ARGS );
static int syscall_cspace_cap_remove( SYSCALL_ARGS );
static int syscall_cspace_cap_restrict( SYSCALL_ARGS );
static int syscall_cspace_cap_share( SYSCALL_ARGS );
static int syscall_cspace_cap_grant( SYSCALL_ARGS );

static int syscall_interrupt_subscribe( SYSCALL_ARGS );
static int syscall_interrupt_unsubscribe( SYSCALL_ARGS );
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
	syscall_thread_set_pager,
	syscall_thread_continue,
	syscall_thread_stop,

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
	syscall_phys_frame_create,
	syscall_phys_frame_split,
	syscall_phys_frame_join,

	// capability space management syscalls
	syscall_cspace_create,
	syscall_cspace_cap_move,
	syscall_cspace_cap_copy,
	syscall_cspace_cap_remove,
	syscall_cspace_cap_restrict,
	syscall_cspace_cap_share,
	syscall_cspace_cap_grant,

	// other syscalls
	syscall_interrupt_subscribe,
	syscall_interrupt_unsubscribe,
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

// TODO: consider moving these functions to capability.c

static const char *cap_type_strings[] = {
	"[invalid]",
	"syncronous IPC endpoint",
	"asyncronous IPC endpoint",
	"capability space",
	"address space",
	"thread",
	"I/O port",
	"physical memory frame",
	"[reserved]",
};

// check whether the requested permissions are compatible with this object
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

	if ( !cap ){
		debug_printf( "=== %u: invalid object! %u\n", thread->id, object );
		return -C4_ERROR_INVALID_OBJECT;
	}

	if ( cap->type != type && type != CAP_TYPE_NULL ){
		const char *a = cap_type_strings[cap->type];
		const char *b = cap_type_strings[type];
		debug_printf( "=== %u: invalid cap type! have %s, but expected %s\n",
		              thread->id, a, b );
		return -C4_ERROR_INVALID_OBJECT;
	}

	if ( (cap->permissions & permissions) != permissions ){
		debug_printf( "=== %u: invalid permissions!\n", thread->id );
		return -C4_ERROR_PERMISSION_DENIED;
	}

    *entry = cap;
    return C4_ERROR_NONE;
}

// check whether a new object can be added in the current capability space,
// and reserve an entry if so
static int do_newobj_cap_check( thread_t *thread ){
	if ( !thread->cap_space ){
		debug_printf( "- invalid cap space! %u\n", thread->id );
		return -C4_ERROR_PERMISSION_DENIED;
	}

	cap_entry_t *root_entry = cap_space_root_entry( thread->cap_space );

	if (( root_entry->permissions & CAP_MODIFY ) == false ){
		return -C4_ERROR_PERMISSION_DENIED;
	}

	// reserve a null entry in the capability table
	cap_entry_t entry = {
		.type        = CAP_TYPE_RESERVED,
		.permissions = 0,
		.object      = NULL,
	};

	uint32_t object = cap_space_insert( thread->cap_space, &entry );

	// if an entry couldn't be inserted, it's out of space, so return an error.
	if ( object == CAP_INVALID_OBJECT ){
		return -C4_ERROR_ENTITY_FULL;
	}

	return object;
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
	int check = 0;

	// error checking, make sure arguments and capabilities are valid
	if ( !is_user_address( entry ) || !is_user_address( stack ))
	{
		debug_printf( "%s: invalid argument, entry: %p, stack: %p\n",
		              entry, stack );

		return -C4_ERROR_INVALID_ARGUMENT;
	}

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	// TODO: take a reference here
	// thread inherents the parent's address space by default
	addr_space_t *space = cur->addr_space;

	thread = thread_create( entry, space, stack, THREAD_FLAG_USER );
	sched_thread_stop( thread );

	uint32_t object = check;
	cap_entry_t cap_entry = (cap_entry_t){
		.type        = CAP_TYPE_THREAD,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object      = thread,
	};

	cap_space_replace( cur->cap_space, object, &cap_entry );
	sched_add_thread( thread );

	debug_printf( ">> created user thread %u\n", thread->id );
	debug_printf( ">>      entry: %p\n", thread->registers.eip );
	debug_printf( ">>      stack: %p\n", thread->registers.esp );
	debug_printf( ">>    current: %p\n", thread );

	// TODO: return capability space address rather than thread id
	//return thread->id;
	return object;
}

static int syscall_thread_set_addrspace( arg_t thread,
                                         arg_t aspace,
                                         arg_t c, arg_t d )
{
	thread_t *target = NULL;
	addr_space_t *new_aspace = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( target, thread, CAP_TYPE_THREAD, CAP_MODIFY );
	CAP_CHECK( new_aspace, aspace, CAP_TYPE_ADDR_SPACE, CAP_ACCESS );

	thread_set_addrspace( target, new_aspace );
	// TODO: check to see if the calling thread is the target thread,
	//       and switch to the new address space before returning

	return C4_ERROR_NONE;
}

static int syscall_thread_set_pager( arg_t thread, arg_t queue,
                                     arg_t c, arg_t d )
{
	thread_t *target = NULL;
	msg_queue_t *endpoint = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( target, thread, CAP_TYPE_THREAD, CAP_MODIFY );
	CAP_CHECK( endpoint, queue, CAP_TYPE_IPC_SYNC_ENDPOINT,
	           CAP_ACCESS | CAP_MODIFY );

	// TODO: locking
	target->pager = endpoint;

	return C4_ERROR_NONE;
}

static int syscall_thread_continue( arg_t thread,
                                    arg_t b, arg_t c, arg_t d )
{
	thread_t *target = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( target, thread, CAP_TYPE_THREAD, CAP_MODIFY );
	sched_thread_continue( target );

	return C4_ERROR_NONE;
}

static int syscall_thread_stop( arg_t thread,
                                arg_t b, arg_t c, arg_t d )
{
	thread_t *target = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( target, thread, CAP_TYPE_THREAD, CAP_MODIFY );
	sched_thread_stop( target );

	return C4_ERROR_NONE;
}

static int syscall_thread_set_capspace( arg_t thread,
                                        arg_t aspace,
                                        arg_t c, arg_t d )
{
	thread_t *target = NULL;
	cap_space_t *new_cspace = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( target, thread, CAP_TYPE_THREAD, CAP_MODIFY );
	CAP_CHECK( new_cspace, aspace, CAP_TYPE_CAP_SPACE, CAP_ACCESS );

	thread_set_capspace( target, new_cspace );

	return C4_ERROR_NONE;
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
	thread_t *cur = sched_current_thread();
	int check = 0;

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	uint32_t object = check;
	msg_queue_t *msgq = message_queue_create();
	cap_entry_t entry = {
		.type = CAP_TYPE_IPC_SYNC_ENDPOINT,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object = msgq,
	};

	cap_space_replace( cur->cap_space, object, &entry );

	return object;
}

static int syscall_sync_send( arg_t buffer, arg_t target, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	msg_queue_t *queue;

	CAP_CHECK_INIT();
	CAP_CHECK( queue, target, CAP_TYPE_IPC_SYNC_ENDPOINT, CAP_MODIFY );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	message_send( queue, msg );

	return 0;
}

static int syscall_sync_recieve( arg_t buffer, arg_t from, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	msg_queue_t *queue;

	CAP_CHECK_INIT();
	CAP_CHECK( queue, from, CAP_TYPE_IPC_SYNC_ENDPOINT, CAP_ACCESS );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	message_recieve( queue, msg );

	return 0;
}

static int syscall_async_create( SYSCALL_ARGS ){
	thread_t *cur = sched_current_thread();
	int check = 0;

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	uint32_t object = check;
	msg_queue_async_t *msgq = message_queue_async_create();
	cap_entry_t entry = {
		.type = CAP_TYPE_IPC_ASYNC_ENDPOINT,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object = msgq,
	};

	cap_space_replace( cur->cap_space, object, &entry );

	return object;
}

static int syscall_async_send( arg_t buffer, arg_t to, arg_t c, arg_t d ){
	message_t *msg = (message_t *)buffer;
	msg_queue_async_t *queue;

	CAP_CHECK_INIT();
	CAP_CHECK( queue, to, CAP_TYPE_IPC_ASYNC_ENDPOINT, CAP_MODIFY );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	return message_send_async( queue, msg );
}

static int syscall_async_recieve( arg_t buffer,
                                  arg_t from,
                                  arg_t flags,
                                  arg_t d )
{
	message_t *msg = (message_t *)buffer;
	msg_queue_async_t *queue;

	CAP_CHECK_INIT();
	CAP_CHECK( queue, from, CAP_TYPE_IPC_ASYNC_ENDPOINT, CAP_ACCESS );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );

		return false;
	}

	return message_recieve_async( queue, msg, flags );
}

static int syscall_addrspace_create( SYSCALL_ARGS ){
	thread_t *cur = sched_current_thread();
	int check = 0;

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	uint32_t object = check;
	addr_space_t *addr = addr_space_clone( addr_space_kernel( ));
	cap_entry_t entry = (cap_entry_t){
		.type        = CAP_TYPE_ADDR_SPACE,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object      = addr,
	};

	cap_space_replace( cur->cap_space, object, &entry );

	return object;
}

static int syscall_addrspace_map( arg_t addrspace,
                                  arg_t phys_frame,
                                  arg_t address,
                                  arg_t permissions )
{
	addr_space_t *space = NULL;
	phys_frame_t *frame = NULL;
	thread_t *cur = sched_current_thread();

	CAP_CHECK_INIT();
	CAP_CHECK( space, addrspace, CAP_TYPE_ADDR_SPACE, CAP_MODIFY );
	CAP_CHECK( frame, phys_frame, CAP_TYPE_PHYS_MEMORY, CAP_ACCESS );

	addr_entry_t ent;

	addr_space_set( space );
	addr_space_make_ent( &ent, address, permissions, frame );
	// TODO: implement addr_space_map() which will check for overlapping
	//       mappings
	addr_space_insert_map( space, &ent );
	addr_space_set( cur->addr_space );

	return C4_ERROR_NONE;
}

static int syscall_addrspace_unmap( arg_t addrspace,
                                    arg_t address,
                                    arg_t c, arg_t d )
{
	addr_space_t *space = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( space, addrspace, CAP_TYPE_ADDR_SPACE, CAP_MODIFY );

	return addr_space_unmap(space, address);
}

// TODO: DANGER: this needs permission checking with a not-yet-implemented
//       context capability to make sure random things can't map arbitrary
//       physical memory
static int syscall_phys_frame_create( arg_t phys_addr,
                                      arg_t size,
                                      arg_t context,
                                      arg_t d )
{
	int check = 0;
	thread_t *cur = sched_current_thread();

	CAP_CHECK_INIT();

	if ((check = do_newobj_cap_check(cur)) < 0) {
		return check;
	}

	uint32_t object = check;
	phys_frame_t *newframe = phys_frame_create(phys_addr, size,
	                                           PHYS_FRAME_FLAG_NONFREEABLE);

	if (!newframe) {
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	// TODO: take a reference to newframe
	cap_entry_t entry = (cap_entry_t){
		.type        = CAP_TYPE_PHYS_MEMORY,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object      = newframe,
	};

	cap_space_replace(cur->cap_space, object, &entry);
	return object;
}


static int syscall_phys_frame_split( arg_t phys_frame,
                                     arg_t offset,
                                     arg_t c,
                                     arg_t d )
{
	phys_frame_t *frame = NULL;
	int check = 0;
	thread_t *cur = sched_current_thread();

	CAP_CHECK_INIT();
	CAP_CHECK( frame, phys_frame, CAP_TYPE_PHYS_MEMORY, CAP_MODIFY );

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	uint32_t object = check;
	phys_frame_t *newframe = phys_frame_split( frame, offset );

	if ( !newframe ){
		return -C4_ERROR_INVALID_ARGUMENT;
	}

	// TODO: take a reference to newframe
	cap_entry_t entry = (cap_entry_t){
		.type        = CAP_TYPE_PHYS_MEMORY,
		.permissions = cap_ent->permissions,
		.object      = newframe,
	};

	cap_space_replace( cur->cap_space, object, &entry );
	return object;
}

static int syscall_phys_frame_join( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_create( SYSCALL_ARGS ){
	thread_t *cur = sched_current_thread();
	int check = 0;

	if (( check = do_newobj_cap_check( cur )) < 0 ){
		return check;
	}

	uint32_t object = check;
	cap_space_t *cspace = cap_space_create();
	cap_entry_t entry = {
		.type = CAP_TYPE_CAP_SPACE,
		.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
		.object = cspace,
	};

	cap_space_replace( cur->cap_space, object, &entry );

	return object;
}

static int syscall_cspace_cap_move( arg_t src,  arg_t srcobj,
                                    arg_t dest, arg_t destobj )
{
	cap_space_t *destspace;
	cap_entry_t *srcent;

	__attribute__((unused)) cap_space_t *srcspace;
	__attribute__((unused)) void *asdfptr;

	CAP_CHECK_INIT();
	CAP_CHECK(srcspace, src, CAP_TYPE_CAP_SPACE, CAP_MODIFY | CAP_SHARE);
	CAP_CHECK(destspace, dest, CAP_TYPE_CAP_SPACE, CAP_MODIFY);
	CAP_CHECK(asdfptr, destobj, CAP_TYPE_NULL, 0);
	CAP_CHECK(asdfptr, srcobj, CAP_TYPE_NULL, CAP_SHARE);
	srcent = cap_ent;

	// possible optimization here: cap_space_replace() will do another lookup
	// of the object in the destination cap space, despite already having
	// a pointer to the entry that will be overwritten here. For now I'm
	// leaving it like this to make implementing references and garbage
	// collection easier in the future.
	cap_space_replace(destspace, destobj, srcent);
	cap_space_remove(srcspace, srcobj);

	return C4_ERROR_NONE;
}

static int syscall_cspace_cap_copy( arg_t src,  arg_t srcobj,
                                    arg_t dest, arg_t destobj )
{
	cap_space_t *destspace;
	cap_entry_t *srcent;

	__attribute__((unused)) cap_space_t *srcspace;
	__attribute__((unused)) void *asdfptr;

	CAP_CHECK_INIT();
	CAP_CHECK(srcspace, src, CAP_TYPE_CAP_SPACE, CAP_MODIFY | CAP_SHARE);
	CAP_CHECK(destspace, dest, CAP_TYPE_CAP_SPACE, CAP_MODIFY);
	CAP_CHECK(asdfptr, destobj, CAP_TYPE_NULL, 0);
	CAP_CHECK(asdfptr, srcobj, CAP_TYPE_NULL, CAP_SHARE);
	srcent = cap_ent;

	// possible optimization here: cap_space_replace() will do another lookup
	// of the object in the destination cap space, despite already having
	// a pointer to the entry that will be overwritten here. For now I'm
	// leaving it like this to make implementing references and garbage
	// collection easier in the future.
	cap_space_replace(destspace, destobj, srcent);

	return C4_ERROR_NONE;
}

static int syscall_cspace_cap_remove( arg_t capspace, arg_t object,
                                      arg_t c, arg_t d )
{
	cap_space_t *cspace;
	__attribute__((unused)) void *aptr;

	CAP_CHECK_INIT();
	CAP_CHECK(cspace, capspace, CAP_TYPE_CAP_SPACE, CAP_MODIFY);
	CAP_CHECK(aptr, object, CAP_TYPE_NULL, 0);

	if (cap_space_remove(cspace, object) == false) {
		return -C4_ERROR_PERMISSION_DENIED;
	}

	return C4_ERROR_NONE;
}

static int syscall_cspace_cap_restrict( arg_t capspace,
                                        arg_t object,
                                        arg_t permissions,
                                        arg_t d )
{
	cap_space_t *cspace;

	CAP_CHECK_INIT();
	CAP_CHECK( cspace, capspace, CAP_TYPE_CAP_SPACE, CAP_MODIFY );

	if ( !cap_space_restrict( cspace, object, permissions )){
		return -C4_ERROR_PERMISSION_DENIED;
	}

	return C4_ERROR_NONE;
}

static int syscall_cspace_cap_share( SYSCALL_ARGS ){
	return -C4_ERROR_NOT_IMPLEMENTED;
}

static int syscall_cspace_cap_grant( arg_t object,
                                     arg_t queue,
                                     arg_t permissions,
                                     arg_t d )
{
	thread_t *cur = sched_current_thread();
	cap_entry_t *cap = NULL;
	int check = do_cap_check( cur, &cap, queue,
	                          CAP_TYPE_IPC_SYNC_ENDPOINT, CAP_MODIFY );

	if ( check != C4_ERROR_NONE ){
		return check;
	}
	msg_queue_t *endpoint = cap->object;

	check = do_cap_check( cur, &cap, object, CAP_TYPE_NULL,
	                      permissions | CAP_SHARE );

	if ( check != C4_ERROR_NONE ){
		return check;
	}

	cap_entry_t temp = *cap;
	temp.permissions = permissions;
	// TODO: error out somehow if the capability could not actually be sent
	//       due to a full capability space on the reciever's end
	message_send_capability( endpoint, &temp );

	return -C4_ERROR_NONE;
}

static int syscall_interrupt_subscribe( arg_t interrupt,
                                        arg_t endpoint,
                                        arg_t c,
                                        arg_t d )
{
	msg_queue_async_t *queue = NULL;

	CAP_CHECK_INIT();
	CAP_CHECK( queue, endpoint, CAP_TYPE_IPC_ASYNC_ENDPOINT, CAP_MODIFY );

	interrupt_listen( interrupt, queue );

	return C4_ERROR_NONE;
}

static int syscall_interrupt_unsubscribe( arg_t endpoint,
                                          arg_t b,
                                          arg_t c,
                                          arg_t d )
{
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
