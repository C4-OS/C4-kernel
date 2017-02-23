#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/interrupts.h>
#include <c4/klib/string.h>
#include <c4/arch/scheduler.h>
#include <c4/mm/slab.h>
#include <stdbool.h>

static inline bool is_kernel_msg( message_t *msg ){
	return msg->type < MESSAGE_TYPE_END_RESERVED;
}

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target );
static inline bool kernel_msg_handle_recieve( message_t *msg );

static inline void set_sender_state( thread_t *sender ){
	if ( FLAG( sender, THREAD_FLAG_FAULTED )){
		sender->state = SCHED_STATE_STOPPED;

	} else {
		sender->state = SCHED_STATE_RUNNING;
	}
}

void message_recieve( message_t *msg, unsigned from ){
	thread_t *cur = sched_current_thread( );

retry:
	if ( FLAG(cur, THREAD_FLAG_PENDING_MSG) == 0 ){
		thread_t *sender = thread_list_pop( &cur->waiting );

		// if there's a thread in the queue, copy it's message to the buffer
		// and requeue it in the scheduler
		if ( sender ){
			cur->message = sender->message;
			set_sender_state( sender );
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

	cur->state = SCHED_STATE_RUNNING;
	UNSET_FLAG( cur, THREAD_FLAG_PENDING_MSG );
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
	   && FLAG(thread, THREAD_FLAG_PENDING_MSG) == 0 )
	{
		thread->message = *msg;
		SET_FLAG( thread, THREAD_FLAG_PENDING_MSG );
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
	thread_t *cur    = sched_current_thread( );
	thread_t *thread = thread_get_id( id );

	if ( !message_try_send( msg, id )){

		if ( thread ){
			cur->message = *msg;
			cur->state   = SCHED_STATE_SENDING;

			thread_list_remove( &cur->sched );
			thread_list_insert( &thread->waiting, &cur->sched );
			sched_thread_yield( );
		}

	} else {
		set_sender_state( cur );
	}
}

static inline void message_queue_insert( message_queue_t *queue,
                                          message_node_t  *node )
{
	KASSERT( node != NULL );

	if ( queue->first ){
		queue->last->next = node;
		queue->last       = node;

	} else {
		queue->first = queue->last = node;
	}

	queue->elements++;
	node->next = NULL;
}

static inline message_node_t *message_queue_remove( message_queue_t *queue ){
	message_node_t *ret = NULL;

	if ( queue->first ){
		ret = queue->first;
		queue->first = queue->first->next;

		if ( !queue->first ){
			queue->last = NULL;
		}

		queue->elements--;
	}

	return ret;
}

static slab_t message_node_slab;

static message_node_t *message_node_alloc( message_t *msg ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &message_node_slab, region_get_global(),
		              sizeof( message_node_t ), NULL, NULL );

		initialized = true;
	}

	message_node_t *ret = slab_alloc( &message_node_slab );
	KASSERT( ret != NULL );

	ret->message = *msg;

	return ret;
}

static void message_node_free( message_node_t *node ){
	slab_free( &message_node_slab, node );
}

bool message_send_async( message_t *msg, unsigned to ){
	thread_t *target  = thread_get_id( to );
	thread_t *current = sched_current_thread( );

	if ( !target ){
		debug_printf( "[ipc] invalid message target, %u -> %u, returning\n",
		              current->id, to );
		return false;
	}

	if ( target->async_queue.elements >= MESSAGE_MAX_QUEUE_ELEMENTS ){
		debug_printf( "[ipc] async queue full, can't send from %u -> %u\n",
		              current->id, to );
		return false;
	}

	// TODO: capability checks, once implemented
	message_queue_insert( &target->async_queue, message_node_alloc( msg ));

	if ( target->state == SCHED_STATE_WAITING_ASYNC ){
		target->state = SCHED_STATE_RUNNING;
	}

	return true;
}

bool message_recieve_async( message_t *msg, unsigned flags ){
	thread_t *current = sched_current_thread( );
	message_node_t *node;

retry:
	node = message_queue_remove( &current->async_queue );

	if ( node ){
		*msg = node->message;
		message_node_free( node );
		return true;

	} else if ( flags & MESSAGE_ASYNC_BLOCK ){
		// same as message_recieve(), the sender will set the thread's state
		// to 'running' whenever they get around to sending a message
		current->state = SCHED_STATE_WAITING_ASYNC;
		sched_thread_yield( );
		goto retry;
	}

	return false;
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

static inline bool message_unmap( message_t *msg, thread_t *target ){
	// TODO: capability checks to see if the current thread can send thread
	//       unmapping messages to the target thread

	thread_t *cur = sched_current_thread( );
	uintptr_t addr = msg->data[0];
	bool should_send = false;

	if ( target->state == SCHED_STATE_STOPPED ){
		addr_space_set( target->addr_space );
		addr_space_unmap( target->addr_space, addr );
		addr_space_set( cur->addr_space );

	} else {
		should_send = true;
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
		// output one character to the debug log
		case MESSAGE_TYPE_DEBUG_PUTCHAR:
			debug_putchar( msg->data[0] );
			break;

		// intercepts message and prints, without sending to the reciever
		case MESSAGE_TYPE_DEBUG_PRINT:
			debug_printf(
				"[ipc] debug message, from thread %u\n"
				"      data[0]: 0x%x\n"
				"      data[1]: 0x%x\n"
				"      data[2]: 0x%x\n",
				current->id, msg->data[0], msg->data[1], msg->data[2] );
			break;

		case MESSAGE_TYPE_DUMP_MAPS:
			debug_printf(
				"[ipc] dumping memory maps for thread %u (map: %p)\n",
				target->id, target->addr_space
			);

			addr_map_dump( target->addr_space->map );
			break;

		case MESSAGE_TYPE_UNMAP:
			should_send = message_unmap( msg, target );
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

		case MESSAGE_TYPE_PAGE_FAULT:
			should_send = true;
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

		case MESSAGE_TYPE_INTERRUPT_SUBSCRIBE:
			interrupt_listen( msg->data[0], current );
			break;

		case MESSAGE_TYPE_INTERRUPT_UNSUBSCRIBE:
			// TODO: unsubscribe function
			break;

		case MESSAGE_TYPE_SET_PAGER:
			target->pager = msg->data[0];
			break;

		default:
			break;
	}

	return should_send;
}

static inline bool kernel_msg_handle_recieve( message_t *msg ){
	switch ( msg->type ){
		case MESSAGE_TYPE_UNMAP:
			{
				thread_t *cur = sched_current_thread( );
				addr_space_unmap( cur->addr_space, msg->data[0] );
			}

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
