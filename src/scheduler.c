#include <c4/scheduler.h>
#include <c4/klib/string.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>

static thread_list_t sched_list;
static thread_t *current_thread;

// TODO: once SMP is working, each CPU will need its own idle thread
static thread_t *global_idle_thread = NULL;

static void idle_thread( void *data ){
	for (;;) {
		asm volatile ( "hlt" );
	}
}

void init_scheduler( void ){
	memset( &sched_list, 0, sizeof(thread_list_t) );
	//sched_add_thread( thread_create_kthread( idle_thread, NULL ));
	global_idle_thread = thread_create_kthread( idle_thread, NULL );

	current_thread = NULL;
}

static inline thread_t *next_thread( thread_t *thread ){
	thread_t *foo = thread;

	if ( foo && foo->sched.next ){
		foo = foo->sched.next->thread;

	} else {
		foo = sched_list.first->thread;
	}

	return foo;
}

// TODO: rewrite scheduler to use a proper priority queue, move blocked
//       threads to seperate lists/queues/whatever
void sched_switch_thread( void ){
	thread_t *next;
	thread_t *start;

	if ( !current_thread || current_thread == global_idle_thread ){
		next  = sched_list.first->thread;
		start = next;

	} else {
		next  = current_thread;
		start = current_thread;
	}

	// TODO: move threads to a seperate 'waiting' list
	while ( next->state != SCHED_STATE_RUNNING ){
		next = next_thread( next );

		if ( next == start ){
			break;
		}
	}

	if ( next->state != SCHED_STATE_RUNNING ){
		sched_jump_to_thread( global_idle_thread );

	} else {
		sched_jump_to_thread( next );
	}
}

void kernel_stack_set( void *addr );
void *kernel_stack_get( void );

void sched_jump_to_thread( thread_t *thread ){
	thread_t *cur = current_thread;
	current_thread = thread;

	if ( !cur || thread->addr_space != cur->addr_space ){
		// XXX: cause a page fault if the address of the new page
		//      directory isn't mapped here, so it can be mapped in
		//      the page fault handler (and not cause a triple fault)
		volatile char *a = (char *)thread->addr_space->page_dir;
		volatile char  b = *a;
		// and keep compiler from complaining about used variable
		b = b;

		//set_page_dir( thread->addr_space->page_dir );
		addr_space_set( thread->addr_space );
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
	thread_list_insert( &sched_list, &thread->sched );
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

	thread_list_remove( &current_thread->sched );
	current_thread = NULL;

	sched_thread_yield( );

	for (;;);

}

thread_t *sched_current_thread( void ){
	return current_thread;
}
