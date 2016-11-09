#include <c4/syscall.h>
#include <c4/debug.h>
#include <c4/message.h>
#include <c4/thread.h>
#include <c4/scheduler.h>

typedef int (*syscall_func_t)( uintptr_t a, uintptr_t b, uintptr_t c );

static int syscall_exit( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_create_thread( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_send( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_send_async( uintptr_t a, uintptr_t b, uintptr_t c );
static int syscall_recieve( uintptr_t a, uintptr_t b, uintptr_t c );

static const syscall_func_t syscall_table[SYSCALL_MAX] = {
	syscall_exit,
	syscall_create_thread,
	syscall_send,
	syscall_send_async,
	syscall_recieve,
};

int syscall_dispatch( unsigned num, uintptr_t a, uintptr_t b, uintptr_t c ){
	if ( num >= SYSCALL_MAX ){
		return -1;
	}

	return syscall_table[num](a, b, c);
}

static int syscall_exit( uintptr_t a, uintptr_t b, uintptr_t c ){
	debug_printf( "got exit with %u, %u, and %u\n", a, b, c );

	return 0;
}

static int syscall_create_thread( uintptr_t user_entry,
                                  uintptr_t user_stack,
                                  uintptr_t user_data )
{
	void (*entry)(void *) = (void *)user_entry;
	void *stack           = (void *)user_stack;
	void *data            = (void *)user_data;

	thread_t *thread;
	thread_t *cur = sched_current_thread( );

	if ( !is_user_address( entry )
	     || !is_user_address( stack )
	     || !is_user_address( data ))
	{
		debug_printf( "%s: invalid argument, entry: %p, stack: %p, data:%p\n",
		              entry, stack, data );

		return -1;
	}

	thread = thread_create( entry, data, cur->page_dir,
	                        stack, THREAD_FLAG_USER );

	sched_add_thread( thread );

	debug_printf( ">> created user thread %u\n", thread->id );
	debug_printf( ">>      entry: %p\n", thread->registers.eip );
	debug_printf( ">>      stack: %p\n", thread->registers.esp );
	debug_printf( ">>       next: %p\n", thread->next );
	debug_printf( ">>       prev: %p\n", thread->prev );
	debug_printf( ">> next->prev: %p\n", thread->next->prev );
	debug_printf( ">>    current: %p\n", thread );

	return thread->id;
}

static int syscall_send( uintptr_t buffer, uintptr_t target, uintptr_t c ){
	message_t *msg = (message_t *)buffer;
	debug_printf( "trying to send message %p to %u\n", msg, target );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -1;
	}

	message_send( msg, target );

	return 0;
}

static int syscall_send_async( uintptr_t a, uintptr_t b, uintptr_t c ){
	return 0;
}

static int syscall_recieve( uintptr_t buffer, uintptr_t b, uintptr_t c ){
	message_t *msg = (message_t *)buffer;
	debug_printf( "trying to recieve message at %p\n", msg );

	if ( !is_user_address( msg )){
		debug_printf( "%s: (invalid buffer, returning)\n", __func__ );
		return -1;
	}

	message_recieve( msg );

	return 0;
}

