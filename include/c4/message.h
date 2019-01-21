#ifndef _C4_MESSAGE_H
#define _C4_MESSAGE_H 1
#include <stdint.h>
#include <stdbool.h>

enum {
	MESSAGE_TYPE_NOP,
	//MESSAGE_TYPE_DEBUG_PUTCHAR,
	MESSAGE_TYPE_DEBUG_PRINT,

	// capability messages
	MESSAGE_TYPE_GRANT_OBJECT,

	// memory control messages
	MESSAGE_TYPE_MAP,
	MESSAGE_TYPE_MAP_TO,
	MESSAGE_TYPE_UNMAP,
	MESSAGE_TYPE_GRANT,
	MESSAGE_TYPE_GRANT_TO,
	MESSAGE_TYPE_REQUEST_PHYS,
	MESSAGE_TYPE_PAGE_FAULT,
	MESSAGE_TYPE_DUMP_MAPS,

	// thread control messages
	MESSAGE_TYPE_STOP,
	MESSAGE_TYPE_CONTINUE,
	MESSAGE_TYPE_END,
	MESSAGE_TYPE_KILL,
	MESSAGE_TYPE_SET_PAGER,

	// hardware interface messages
	MESSAGE_TYPE_INTERRUPT,
	MESSAGE_TYPE_INTERRUPT_SUBSCRIBE,
	MESSAGE_TYPE_INTERRUPT_UNSUBSCRIBE,

	// end of kernel-reserved ipc types, users can define their own
	// types after this.
	MESSAGE_TYPE_END_RESERVED = 0x100,
};

// flags for asyncronous messages, used at message_recieve_async
enum {
	MESSAGE_ASYNC_BLOCK = 1,
};

// various constants
enum {
	MESSAGE_MAX_QUEUE_ELEMENTS = 64,
};

typedef struct message {
	unsigned type;
	unsigned sender;
	unsigned long data[6];
} message_t;

typedef struct msg_queue msg_queue_t;
typedef struct msg_queue_async msg_queue_async_t;

#include <c4/thread.h>
#include <c4/capability.h>
#include <c4/kobject.h>

// TODO: consider implementing generic linked list structure, or macro library
//       or something
typedef struct message_node {
	struct message_node *next;

	message_t message;
} message_node_t;

typedef struct msg_queue {
	kobject_t object;

	thread_list_t recievers;
	thread_list_t senders;
} msg_queue_t;

typedef struct msg_queue_async {
	kobject_t object;

	thread_list_t  recievers;
	message_node_t *first;
	message_node_t *last;

	unsigned elements;
} msg_queue_async_t;

// non-locking
msg_queue_t *message_queue_create( void );
msg_queue_async_t *message_queue_async_create( void );
void message_queue_free( msg_queue_t *queue );
void message_queue_async_free( msg_queue_async_t *queue );

bool message_try_send( msg_queue_t *queue, message_t *msg );
void message_send_capability( msg_queue_t *queue, cap_entry_t *cap );

// locking
void message_recieve( msg_queue_t *queue, message_t *msg );
void message_send( msg_queue_t *queue, message_t *msg );

bool message_send_async( msg_queue_async_t *queue, message_t *msg );
bool message_recieve_async( msg_queue_async_t *queue, message_t *msg, unsigned flags );

#endif
