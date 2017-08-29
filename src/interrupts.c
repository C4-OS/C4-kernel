#include <c4/arch/interrupts.h>
#include <c4/interrupts.h>
#include <c4/thread.h>
#include <c4/scheduler.h>
#include <c4/debug.h>
#include <stddef.h>

//static unsigned listening_threads[INTERRUPT_MAX];
static msg_queue_async_t *listening_endpoints[INTERRUPT_MAX];

bool interrupt_callback( unsigned num, unsigned flags ){
	if ( listening_endpoints[num] == NULL ){
		return false;
	}

	//thread_t *thread = thread_get_id( listening_threads[num] );
	msg_queue_async_t *queue = listening_endpoints[num];

	message_t msg = {
		.type = MESSAGE_TYPE_INTERRUPT,
		.data = { num, },
	};

	message_send_async( queue, &msg );
	//message_send_async( &msg, thread->id );
	// TODO: send to an endpoint, once that's implemented

	return true;
}

//int interrupt_listen( unsigned num, thread_t *thread ){
int interrupt_listen( unsigned num, msg_queue_async_t *queue ){
	if ( num >= INTERRUPT_MAX ){
		// TODO: make error number header to return proper errors
		return -1;
	}

	debug_printf( "got here, %p listening for interrupt %u\n", queue, num );

	// TODO: store a list of endpoints to send a message to
	// TODO: take a reference here
	listening_endpoints[num] = queue;

	return 0;
}
