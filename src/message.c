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

// check if the target thread can currently recieve this message
static inline bool message_thread_can_recieve( thread_t *target ){
	//thread_t *cur = sched_current_thread( );

	return target->state == SCHED_STATE_WAITING
	    && FLAG(target, THREAD_FLAG_PENDING_MSG) == false;
}

void message_recieve(msg_queue_t *queue, message_t *msg) {
	thread_t *cur = sched_current_thread();

retry:
	kobject_lock(&queue->object);

	if (FLAG(cur, THREAD_FLAG_PENDING_MSG) == 0) {
		thread_t *sender = thread_list_pop(&queue->senders);

		// if there's a thread in the queue, copy it's message to the buffer
		// and requeue it in the scheduler
		if (sender) {
			cur->message = sender->message;
			set_sender_state(sender);
			sched_add_thread(sender);

		// otherwise block the thread and wait for a message to be recieved.
		// since the state is set to 'waiting', it won't be run again
		// until a message is recieved
		} else {
			cur->state = SCHED_STATE_WAITING;

			thread_list_remove(&cur->sched);
			thread_list_insert(&queue->recievers, &cur->sched);

			kobject_unlock(&queue->object);
			sched_thread_yield();
			goto retry;
		}
	}

	if (is_kernel_msg(&cur->message)) {
		kernel_msg_handle_recieve(&cur->message);
	}

	cur->state = SCHED_STATE_RUNNING;
	UNSET_FLAG(cur, THREAD_FLAG_PENDING_MSG);
	*msg = cur->message;

	kobject_unlock(&queue->object);
}

// NOTE: `queue` should be locked by the caller
bool message_try_send(msg_queue_t *queue, message_t *msg) {
	thread_t *cur    = sched_current_thread();
	// TODO: lock thread
	thread_t *thread = thread_list_peek(&queue->recievers);

	if (!thread) {
		return false;
	}

	// handle kernel interface messages
	if (is_kernel_msg(msg)) {
		bool should_send = kernel_msg_handle_send(msg, thread);

		if (!should_send) {
			return true;
		}
	}

	// set sender field
	msg->sender = cur->id;

	if (message_thread_can_recieve(thread)) {
		SET_FLAG(thread, THREAD_FLAG_PENDING_MSG);

		thread_list_remove(&thread->sched);
		thread->state = SCHED_STATE_RUNNING;
		thread->message = *msg;
		sched_add_thread(thread);

		return true;
	}

	return false;
}

void message_send(msg_queue_t *queue, message_t *msg) {
	// try to copy the message buffer to the target thread,
	// or put thread into the target's waiting list if it can't.
	//
	// the target then copies the message buffer from the sender thread
	// once the target does a message_recieve() call, and this thread is
	// popped from the list.
	thread_t *cur    = sched_current_thread();
	kobject_lock(&queue->object);

	if (!message_try_send(queue, msg)) {
		cur->message = *msg;
		cur->state   = SCHED_STATE_SENDING;

		thread_list_remove(&cur->sched);
		thread_list_insert(&queue->senders, &cur->sched);
		kobject_unlock(&queue->object);
		sched_thread_yield();

	} else {
		set_sender_state(cur);
		kobject_unlock(&queue->object);
	}
}

void message_send_capability(msg_queue_t *queue, cap_entry_t *cap) {
	message_t msg = {
		.type = MESSAGE_TYPE_GRANT_OBJECT,
		.data = {
			cap->type,
			cap->permissions,
			(uint32_t)cap->object,
		},
	};

	message_send(queue, &msg);
}

static inline void message_queue_async_insert( msg_queue_async_t *queue,
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

static inline message_node_t *message_queue_async_remove( msg_queue_async_t *queue )
{
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
static slab_t message_queue_slab;
static slab_t message_queue_async_slab;

msg_queue_t *message_queue_create( void ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &message_queue_slab, region_get_global(),
		              sizeof( msg_queue_t ), NO_CTOR, NO_DTOR );

		initialized = true;
	}

	msg_queue_t *ret = slab_alloc( &message_queue_slab );
	KASSERT( ret != NULL );

	return ret;
}

msg_queue_async_t *message_queue_async_create( void ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &message_queue_async_slab, region_get_global(),
		              sizeof( msg_queue_t ), NO_CTOR, NO_DTOR );

		initialized = true;
	}

	msg_queue_async_t *ret = slab_alloc( &message_queue_async_slab );
	KASSERT( ret != NULL );

	return ret;
}

void message_queue_free( msg_queue_t *queue ){

}

void message_queue_async_free( msg_queue_async_t *queue ){

}

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

bool message_send_async(msg_queue_async_t *queue, message_t *msg) {
	// TODO: lock thread
	thread_t *current = sched_current_thread();
	kobject_lock(&queue->object);

	if (queue->elements >= MESSAGE_MAX_QUEUE_ELEMENTS) {
		debug_printf("[ipc] async queue full, can't send from %u\n",
		              current->id);

		kobject_unlock(&queue->object);
		return false;
	}

	message_queue_async_insert(queue, message_node_alloc(msg));

	// if there are blocked threads on this endpoint, wake one up
	// so that it can recieve a message
	thread_t *target = thread_list_pop(&queue->recievers);
	if (target) {
		target->state = SCHED_STATE_RUNNING;
		sched_add_thread(target);
	}

	kobject_unlock(&queue->object);
	return true;
}

bool message_recieve_async(msg_queue_async_t *queue,
                           message_t *msg,
                           unsigned flags)
{
	// TODO: lock thread
	thread_t *current = sched_current_thread();
	message_node_t *node;

retry:
	kobject_lock(&queue->object);
	node = message_queue_async_remove(queue);

	if (node) {
		*msg = node->message;
		message_node_free(node);
		kobject_unlock(&queue->object);
		return true;

	} else if (flags & MESSAGE_ASYNC_BLOCK) {
		// same as message_recieve(), the sender will set the thread's state
		// to 'running' whenever they get around to sending a message
		current->state = SCHED_STATE_WAITING_ASYNC;
		thread_list_remove(&current->sched);
		thread_list_insert(&queue->recievers, &current->sched);

		kobject_unlock(&queue->object);
		sched_thread_yield();
		goto retry;
	}

	kobject_unlock(&queue->object);
	return false;
}

enum {
	MAP_IS_MAP   = false,
	MAP_IS_GRANT = true,
};

static inline bool kernel_msg_handle_send( message_t *msg, thread_t *target ){
	thread_t *current = sched_current_thread( );
	bool should_send = false;

	switch ( msg->type ){
		// intercepts message and prints, without sending to the reciever
		case MESSAGE_TYPE_DEBUG_PRINT:
			debug_printf(
				"[ipc] debug message, from thread %u\n"
				"      data[0]: 0x%x\n"
				"      data[1]: 0x%x\n"
				"      data[2]: 0x%x\n",
				current->id, msg->data[0], msg->data[1], msg->data[2] );
			break;

		case MESSAGE_TYPE_GRANT_OBJECT:
			should_send = true;
			break;

		case MESSAGE_TYPE_PAGE_FAULT:
			should_send = true;
			break;

		default:
			break;
	}

	return should_send;
}

static inline bool kernel_msg_handle_recieve( message_t *msg ){
	switch ( msg->type ){
		case MESSAGE_TYPE_GRANT_OBJECT:
			{
				// TODO: lock thread, capspace
				thread_t *cur = sched_current_thread();
				msg->data[5] = 0;

				if ( !cur->cap_space ){
					return false;
				}

				cap_entry_t entry = {
					.type = msg->data[0],
					.permissions = msg->data[1],
					// TODO: this will need to be a 64 bit pointer on amd64
					//       maybe have this take two fields in the message
					.object = (void *)msg->data[2],
				};

				uint32_t obj = cap_space_insert( cur->cap_space, &entry );
				msg->data[5] = obj;
				msg->data[2] = 0;
			}

		default:
			break;
	}

	return true;
}
