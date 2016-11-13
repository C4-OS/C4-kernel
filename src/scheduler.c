#include <c4/scheduler.h>
#include <c4/klib/string.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>

static thread_list_t sched_list;
static thread_t *current_thread;

static void idle_thread( void *data ){
	for (;;) {
		asm volatile ( "hlt" );
	}
}

void init_scheduler( void ){
	memset( &sched_list, 0, sizeof(thread_list_t) );
	sched_add_thread( thread_create_kthread( idle_thread, NULL ));

	current_thread = NULL;
}

static inline thread_t *next_thread( thread_t *thread ){
	thread_t *foo = thread;

	if ( foo && foo->next ){
		foo = foo->next;

	} else {
		foo = sched_list.first;
	}

	return foo;
}

void sched_switch_thread( void ){
	thread_t *next = next_thread( current_thread );

	// TODO: move threads to a seperate 'waiting' list
	while ( next->state != SCHED_STATE_RUNNING ){
		next = next_thread( next );
	}

	sched_jump_to_thread( next );
}

void kernel_stack_set( void *addr );
void *kernel_stack_get( void );

void sched_jump_to_thread( thread_t *thread ){
	thread_t *cur = current_thread;
	current_thread = thread;

	if ( !cur || thread->page_dir != cur->page_dir ){
		// XXX: cause a page fault if the address of the new page
		//      directory isn't mapped here, so it can be mapped in
		//      the page fault handler (and not cause a triple fault)
		volatile char *a = (char *)thread->page_dir;
		volatile char  b = *a;
		// and keep compiler from complaining about used variable
		b = b;

		set_page_dir( thread->page_dir );
	}

	// swap per-thread kernel stacks
	if ( cur ){
		cur->kernel_stack = kernel_stack_get( );
	}
	kernel_stack_set( thread->kernel_stack );

	sched_do_thread_switch( cur, thread );
}

void sched_thread_yield( void ){
	sched_switch_thread( );
}

void sched_add_thread( thread_t *thread ){
	thread_list_insert( &sched_list, thread );
}

void sched_thread_continue( thread_t *thread ){
	thread->state = SCHED_STATE_RUNNING;
}

void sched_thread_stop( thread_t *thread ){
	thread->state = SCHED_STATE_STOPPED;
}

void sched_thread_exit( void ){
	debug_printf( "got to exit, thread %u\n", current_thread->id );

	// TODO: Add syncronization here, to prevent current_thread from
	//       being left in an inconsistent state if a task switch happens
	//       this will probably crash randomly until it's fixed

	thread_list_remove( current_thread );
	current_thread = NULL;

	sched_thread_yield( );

	for (;;);

}

thread_t *sched_get_thread_by_id( unsigned id ){
	thread_t *temp = sched_list.first;

	for ( ; temp; temp = temp->next ){
		if ( temp->id == id ){
			return temp;
		}
	}

	return NULL;
}

thread_t *sched_current_thread( void ){
	return current_thread;
}
