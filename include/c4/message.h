#ifndef _C4_MESSAGE_H
#define _C4_MESSAGE_H 1
#include <stdint.h>
#include <stdbool.h>

typedef struct message {
	unsigned long data;
} message_t;

void message_recieve( message_t *msg );
bool message_try_send( message_t *msg, unsigned id );
void message_send( message_t *msg, unsigned id );

#endif
