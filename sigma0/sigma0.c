#include <c4/syscall.h>
#include <c4/message.h>
#include <stdbool.h>

#define DO_SYSCALL(N, A, B, C, RET) \
	asm volatile ( " \
		mov %1, %%eax; \
		mov %2, %%edi; \
		mov %3, %%esi; \
		mov %4, %%edx; \
		int $0x60;     \
		mov %%eax, %0  \
	" : "=r"(RET) \
	  : "g"(N), "g"(A), "g"(B), "g"(C) \
	  : "eax", "edi", "esi", "edx" );

void main( void );
void server( void );
int c4_msg_send( message_t *buffer, unsigned target );
int c4_msg_recieve( message_t *buffer, unsigned whom );

void main( void ){
	server( );

	// TODO: panic or dump debug info or something, server()
	//       should never return
	for ( ;; );
}

void server( void ){
	message_t msg;

	while ( true ){
		c4_msg_recieve( &msg, 0 );
		c4_msg_send( &msg, 2 );

		msg.type = MESSAGE_TYPE_DEBUG_PRINT;
		c4_msg_send( &msg, 3 );
	}

	for ( ;; );
}

int c4_msg_send( message_t *buffer, unsigned to ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_SEND, buffer, to, 0, ret );

	return ret;
}

int c4_msg_recieve( message_t *buffer, unsigned from ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_RECIEVE, buffer, from, 0, ret );

	return ret;
}
