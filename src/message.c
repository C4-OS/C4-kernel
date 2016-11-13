#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/debug.h>
#include <c4/arch/scheduler.h>
#include <stdbool.h>

static inline bool is_kernel_msg( message_t *msg ){
	return msg->type < MESSAGE_TYPE_END_RESERVED;
}

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target );
static inline bool kernel_msg_handle_recieve( message_t *msg );

void message_recieve( message_t *msg ){
	volatile thread_t *cur = sched_current_thread( );

	while ( (cur->flags & SCHED_FLAG_PENDING_MSG) == 0 ){
		cur->state = SCHED_STATE_WAITING;

		sched_thread_yield( );
	}

	cur->flags &= ~SCHED_FLAG_PENDING_MSG;
	*msg = cur->message;
}

bool message_try_send( message_t *msg, unsigned id ){
	thread_t *thread = sched_get_thread_by_id( id );
	thread_t *cur    = sched_current_thread( );

	if ( !thread ){
		debug_printf( "[ipc] invalid message target, %u -> %u, returning\n",
		              cur->id, id );
		return true;
	}

	// handle kernel interface messages
	if ( is_kernel_msg( msg )){
		bool return_early = kernel_msg_handle_send( msg, thread );

		if ( return_early ){
			return true;
		}
	}

	// set sender field
	msg->sender = cur->id;

	if ( thread->state == SCHED_STATE_WAITING 
	   && (thread->flags & SCHED_FLAG_PENDING_MSG) == 0 )
	{
		thread->message = *msg;
		thread->flags |= SCHED_FLAG_PENDING_MSG;
		thread->state = SCHED_STATE_RUNNING;

		return true;
	}

	return false;
}

void message_send( message_t *msg, unsigned id ){
	while ( !message_try_send( msg, id )){
		sched_thread_yield( );
	}
}

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target ){
	thread_t *current = sched_current_thread( );

	switch ( msg->type ){
		// intercepts message and prints, without sending to the reciever
		case MESSAGE_TYPE_DEBUG_PRINT:
			debug_printf(
				"[ipc] debug message, from thread %u (task %u)\n"
				"      data[0]: 0x%x\n"
				"      data[1]: 0x%x\n"
				"      data[2]: 0x%x\n",
				current->id, current->task_id,
				msg->data[0], msg->data[1], msg->data[2] );
			break;

		// handle thread control messages
		// TODO: once capabilities are implemented, check for proper
		//       capabilities to send these
		case MESSAGE_TYPE_CONTINUE:
			debug_printf( "continuing thread %u\n", target->id );
			sched_thread_continue( target );
			break;

		case MESSAGE_TYPE_STOP:
			debug_printf( "stopping thread %u\n", target->id );
			sched_thread_stop( target );
			break;

		default:
			break;
	}

	return true;
}

static inline bool kernel_msg_handle_recieve( message_t *msg ){
	return true;
}
