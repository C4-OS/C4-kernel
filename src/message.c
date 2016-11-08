#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/debug.h>
#include <c4/arch/scheduler.h>
#include <stdbool.h>

static inline bool is_kernel_msg( message_t *msg ){
	return msg->type < MESSAGE_TYPE_END_RESERVED;
}

static inline void kernel_msg_handle_send( message_t *msg, thread_t *target );
static inline void kernel_msg_handle_recieve( message_t *msg );

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

	if ( !thread ){
		thread_t *cur = sched_current_thread( );

		debug_printf( "[ipc] invalid message target, %u -> %u, returning\n",
		              cur->id, id );
		return true;
	}

	if ( is_kernel_msg( msg )){
		kernel_msg_handle_send( msg, thread );

		if ( msg->type == MESSAGE_TYPE_DEBUG_PRINT ){
			return true;
		}
	}

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

static inline void kernel_msg_handle_send( message_t *msg, thread_t *target ){
	thread_t *current = sched_current_thread( );

	switch ( msg->type ){
		case MESSAGE_TYPE_DEBUG_PRINT:
			debug_printf(
				"[ipc] debug message, from thread %u (task %u)\n"
				"      data[0]: 0x%x\n"
				"      data[1]: 0x%x\n"
				"      data[2]: 0x%x\n",
				current->id, current->task_id,
				msg->data[0], msg->data[1], msg->data[2] );
			break;

		default:
			break;
	}
}

static inline void kernel_msg_handle_recieve( message_t *msg ){

}
