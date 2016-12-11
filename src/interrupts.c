#include <c4/arch/interrupts.h>
#include <c4/interrupts.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/debug.h>

static thread_list_t listening_threads[INTERRUPT_MAX];

void interrupt_callback( unsigned num, unsigned flags ){
	thread_t *thread = thread_list_pop( listening_threads + num );

	for ( ; thread; thread = thread_list_pop( listening_threads + num )){
		message_t msg = {
			.type = MESSAGE_TYPE_INTERRUPT,
			.data = {
				num,
			}
		};

		debug_printf( "got here, sending interrupt %u to %u\n",
		              num, thread->id );

		// send thread a message and place it back in running list

		// TODO: maybe change message_try_send() to take a thread_t argument
		//       rather than a thread id, so it doesn't have to unnecessarily
		//       do thread lookups
		if ( !message_try_send( &msg, thread->id )){
			debug_printf( "warning: interrupt %u was not sent to thread %u\n",
			              num, thread->id );
		}

		thread->state = SCHED_STATE_RUNNING;
		sched_add_thread( thread );
	}
}

int interrupt_listen( unsigned num, thread_t *thread ){
	if ( num >= INTERRUPT_MAX ){
		// TODO: make error number header to return proper errors
		return -1;
	}

	debug_printf( "got here, thread %u listening for interrupt %u\n",
				  thread->id, num );

	thread_list_remove( &thread->sched );
	thread_list_insert( listening_threads + num, &thread->sched );

	thread->state = SCHED_STATE_WAITING;

	return 0;
}
