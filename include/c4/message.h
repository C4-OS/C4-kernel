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

	// end of kernel-reserved ipc types, users can define their own
	// types after this.
	MESSAGE_TYPE_END_RESERVED = 0x100,
};

typedef struct message {
	unsigned type;
	unsigned sender;
	unsigned long data[6];
} message_t;

void message_recieve( message_t *msg );
bool message_try_send( message_t *msg, unsigned id );
void message_send( message_t *msg, unsigned id );

#endif
