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

static inline thread_t *next_thread( thread_t *thread ){
	thread_t *foo = thread->next;

	if ( !foo ){
		foo = sched_list.first;
	}

	return foo;
}

void sched_switch_thread( void ){
	volatile thread_t *next = next_thread( current_thread );
	volatile bool switched = false;

	// TODO: move threads to a seperate 'waiting' list
	while ( next->state != SCHED_STATE_RUNNING ){
		next = next_thread((thread_t *)next );
	}

	THREAD_SAVE_STATE( current_thread );

	if ( !switched && next != current_thread ){
		switched = true;
		sched_jump_to_thread((thread_t *)next );
	}
}

void sched_jump_to_thread( thread_t *thread ){
	current_thread = thread;

	THREAD_RESTORE_STATE( thread );
}

void sched_add_thread( thread_t *thread ){
	thread_list_insert( &sched_list, thread );
}

void sched_thread_exit( void ){
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
