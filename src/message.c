#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/debug.h>
#include <c4/arch/scheduler.h>
#include <c4/klib/string.h>
#include <stdbool.h>
#include <c4/interrupts.h>

static inline bool is_kernel_msg( message_t *msg ){
	return msg->type < MESSAGE_TYPE_END_RESERVED;
}

static inline bool is_interrupt_msg( unsigned from ){
	return (from & MESSAGE_INTERRUPT_MASK) == MESSAGE_INTERRUPT_MASK;
}

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target );
static inline bool kernel_msg_handle_recieve( message_t *msg );

void message_recieve( message_t *msg, unsigned from ){
	thread_t *cur = sched_current_thread( );

retry:
	// handle interrupt listeners
	if ( is_interrupt_msg( from )){
		interrupt_listen( from - MESSAGE_INTERRUPT_MASK, cur );
		sched_thread_yield( );
	}

	if ( (cur->flags & SCHED_FLAG_PENDING_MSG) == 0 ){
		thread_t *sender = thread_list_pop( &cur->waiting );

		// if there's a thread in the queue, copy it's message to the buffer
		// and requeue it in the scheduler
		if ( sender ){
			cur->message = sender->message;
			sender->state = SCHED_STATE_RUNNING;

			sched_add_thread( sender );

		// otherwise block the thread and wait for a message to be recieved.
		// since the state is set to 'waiting', it won't be run again
		// until a message is recieved
		} else {
			cur->state = SCHED_STATE_WAITING;
			sched_thread_yield( );
			goto retry;
		}
	}

	if ( is_kernel_msg( &cur->message )){
		kernel_msg_handle_recieve( &cur->message );
	}

	cur->state  = SCHED_STATE_RUNNING;
	cur->flags &= ~SCHED_FLAG_PENDING_MSG;
	*msg = cur->message;
}

bool message_try_send( message_t *msg, unsigned id ){
	thread_t *thread = thread_get_id( id );
	thread_t *cur    = sched_current_thread( );

	if ( !thread ){
		debug_printf( "[ipc] invalid message target, %u -> %u, returning\n",
		              cur->id, id );
		return true;
	}

	// handle kernel interface messages
	if ( is_kernel_msg( msg )){
		bool should_send = kernel_msg_handle_send( msg, thread );

		if ( !should_send ){
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
	// try to copy the message buffer to the target thread,
	// or put thread into the target's waiting list if it can't.
	//
	// the target then copies the message buffer from the sender thread
	// once the target does a message_recieve() call, and this thread is
	// popped from the list.
	if ( !message_try_send( msg, id )){
		thread_t *thread = thread_get_id( id );
		thread_t *cur    = sched_current_thread( );

		if ( thread ){
			cur->message = *msg;
			cur->state   = SCHED_STATE_SENDING;

			thread_list_remove( &cur->sched );
			thread_list_insert( &thread->waiting, &cur->sched );
			sched_thread_yield( );
		}
	}
}

bool message_send_async( message_t *msg, unsigned to ){
	return true;
}

void message_recieve_async( message_t *msg, unsigned flags ){

}

enum {
	MAP_IS_MAP   = false,
	MAP_IS_GRANT = true,
};

static inline bool message_map_to( message_t *msg,
                                   thread_t *target,
                                   bool grant )
{
	// TODO: keep track of whether an entry is a map or grant, and refuse
	//       to grant an entry to another address space when the current
	//       address space was given it as a map.
	//       (continuing to map and subdivide maps is ok, though)
	//
	//       rationale being that when all threads referencing an address
	//       space exit, the memory will need to be reclaimed. So, it's assumed
	//       that all maps have a backing grant at some other address space,
	//       and that grants are the `definitive` entry, and when automatic
	//       cleanup occurs, all granted entries would be granted back to the
	//       pager for the address space to be redistributed.
	//
	//       the thread(s) themselves can't release memory back, because they
	//       might have faulted and be unable to continue.
	//
	//       consider ways to handle granting entries away which are currently
	//       mapped in other address spaces, but are in the current address
	//       space as a grant.
	//       this doesn't violate the assumptions above,
	//       but would lead to use-after-frees if the pager is given meory
	//       while another address space continues to use it.
	//       one solution: keep track of the address space which gave the
	//       current address space the entry, and keep reference counts on
	//       entries based off of physical memory addresses.

	unsigned long from   = msg->data[0];
	unsigned long to     = msg->data[1];
	unsigned long size   = msg->data[2];
	unsigned long perms  = msg->data[3];
	bool should_send = false;

	addr_entry_t ent = (addr_entry_t){
		.virtual     = from,
		.size        = size,
		.permissions = perms,
	};

	thread_t *cur = sched_current_thread( );

	addr_entry_t *temp = addr_map_carve( cur->addr_space->map, &ent );
	addr_entry_t msgbuf;

	//memcpy( &msgbuf, temp, sizeof( addr_entry_t ));
	msgbuf = *temp;
	msgbuf.virtual = to;

	if ( temp ){
		if ( grant ){
			addr_space_remove_map( cur->addr_space, temp );
		}

		if ( target->state == SCHED_STATE_STOPPED ){
			addr_space_set( target->addr_space );
			addr_space_insert_map( target->addr_space, &msgbuf );
			addr_space_set( cur->addr_space );

			should_send = false;

		} else {
			addr_entry_t *buf  = (addr_entry_t *)msg->data;
			*buf = msgbuf;
			//memcpy( buf, &msgbuf, sizeof( addr_entry_t ));

			should_send = true;
		}
	}

	return should_send;
}

static inline void message_request_phys( message_t *msg ){
	thread_t *current = sched_current_thread( );

	addr_entry_t ent = (addr_entry_t){
		.virtual     = msg->data[0],
		.physical    = msg->data[1],
		.size        = msg->data[2],
		.permissions = msg->data[3],
	};

	addr_space_insert_map( current->addr_space, &ent );
}

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target ){
	thread_t *current = sched_current_thread( );
	bool should_send = false;

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

		case MESSAGE_TYPE_DUMP_MAPS:
			debug_printf(
				"[ipc] dumping memory maps for thread %u (map: %p)\n",
				target->id, target->addr_space
			);

			addr_map_dump( target->addr_space->map );
			break;

		// memory control messages
		case MESSAGE_TYPE_MAP_TO:
			should_send = message_map_to( msg, target, MAP_IS_MAP );
			break;

		case MESSAGE_TYPE_GRANT_TO:
			should_send = message_map_to( msg, target, MAP_IS_GRANT );
			break;

		case MESSAGE_TYPE_REQUEST_PHYS:
			// TODO: access checks for this once capabilities are implemented
			message_request_phys( msg );
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

		case MESSAGE_TYPE_INTERRUPT:
			should_send = true;
			break;

		default:
			break;
	}

	return should_send;
}

static inline bool kernel_msg_handle_recieve( message_t *msg ){
	switch ( msg->type ){
		case MESSAGE_TYPE_MAP_TO:
		case MESSAGE_TYPE_GRANT_TO:
			{
				thread_t *cur = sched_current_thread( );
				addr_entry_t *ent = (addr_entry_t *)msg->data;

				addr_space_insert_map( cur->addr_space, ent );
			}
			break;

		default:
			break;
	}

	return true;
}
