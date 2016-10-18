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

	sched_add_thread( thread_create( idle_thread, NULL ));

	current_thread = sched_list.first;
}

void sched_switch_thread( void ){
	thread_t *next = current_thread->next;
	volatile bool switched = false;

	if ( !next ){
		next = sched_list.first;
	}

	if ( current_thread->flags & SCHED_FLAG_HAS_RAN ){
		THREAD_SAVE_STATE( current_thread );

	} else {
		current_thread->flags |= SCHED_FLAG_HAS_RAN;
	}

	if ( !switched && next != current_thread ){
		switched = true;
		sched_jump_to_thread( next );
	}
}

void sched_jump_to_thread( thread_t *thread ){
	debug_printf( "switching to thread %u (0x%x)\n", thread->id, thread->registers.eip );
	current_thread = thread;

	THREAD_RESTORE_STATE( thread );
}

void sched_add_thread( thread_t *thread ){
	thread_list_insert( &sched_list, thread );
}

void sched_thread_exit( void ){
	for (;;);
}
