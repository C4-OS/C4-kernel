#include <c4/arch/interrupts.h>
#include <c4/interrupts.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/debug.h>

static unsigned listening_threads[INTERRUPT_MAX];

bool interrupt_callback( unsigned num, unsigned flags ){
	if ( listening_threads[num] == 0 )
		return false;

	thread_t *thread = thread_get_id( listening_threads[num] );

	message_t msg = {
		.type = MESSAGE_TYPE_INTERRUPT,
		.data = {
			num,
		}
	};

	// TODO: maybe change message_send_async() to take a thread_t argument
	//       rather than a thread id, so it doesn't have to unnecessarily
	//       do thread lookups
	message_send_async( &msg, thread->id );

	return true;
}

int interrupt_listen( unsigned num, thread_t *thread ){
	if ( num >= INTERRUPT_MAX ){
		// TODO: make error number header to return proper errors
		return -1;
	}

	debug_printf( "got here, thread %u listening for interrupt %u\n",
				  thread->id, num );

	// TODO: store a list of threads to send a message to
	listening_threads[num] = thread->id;

	return 0;
}
