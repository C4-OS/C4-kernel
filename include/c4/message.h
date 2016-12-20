#ifndef _C4_MESSAGE_H
#define _C4_MESSAGE_H 1
#include <stdint.h>
#include <stdbool.h>

enum {
	MESSAGE_TYPE_NOP,
	MESSAGE_TYPE_DEBUG_PRINT,

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

	// hardware interface messages
	MESSAGE_TYPE_INTERRUPT,

	// end of kernel-reserved ipc types, users can define their own
	// types after this.
	MESSAGE_TYPE_END_RESERVED = 0x100,

	// mask for specifying that an interrupt should be listened for
	MESSAGE_INTERRUPT_MASK    = 0xfff80000,
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

// TODO: consider implementing generic linked list structure, or macro library
//       or something
typedef struct message_node {
	struct message_node *next;

	message_t message;
} message_node_t;

typedef struct message_queue {
	message_node_t *first;
	message_node_t *last;

	unsigned elements;
} message_queue_t;

void message_recieve( message_t *msg, unsigned from );
bool message_try_send( message_t *msg, unsigned id );
void message_send( message_t *msg, unsigned id );

bool message_send_async( message_t *msg, unsigned to );
bool message_recieve_async( message_t *msg, unsigned flags );

#endif
