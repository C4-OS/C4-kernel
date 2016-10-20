#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/arch/scheduler.h>
#include <stdbool.h>

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
	volatile thread_t *thread = sched_get_thread_by_id( id );

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
