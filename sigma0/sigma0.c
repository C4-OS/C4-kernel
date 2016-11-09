#include <c4/syscall.h>
#include <c4/message.h>
#include <stdbool.h>

#define NULL ((void *)0)

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
void server( void * );
void test_thread( void *unused );
int c4_msg_send( message_t *buffer, unsigned target );
int c4_msg_recieve( message_t *buffer, unsigned whom );
int c4_create_thread( void (*entry)(void *), void *stack, void *data );

#define NUM_THREADS 14

struct foo {
	int target;
	int threads[NUM_THREADS];
};

void main( void ){
	unsigned *s = (void *)0xbfffe000;
	struct foo thing;

	thing.target  = 2;

	for ( unsigned i = 0; i < NUM_THREADS; i++ ){
		thing.threads[i] = c4_create_thread( test_thread, s, &thing );
		s -= 1024;
	}

	server( &thing );

	// TODO: panic or dump debug info or something, server()
	//       should never return
	for ( ;; );
}

void test_thread( void *data ){
	message_t msg;
	volatile struct foo *meh = data;

	msg.type = 0xcafe;

	for (;;){
		c4_msg_recieve( &msg, 0 );
		c4_msg_send( &msg, meh->target );
	}
}

void server( void *data ){
	message_t msg;

	volatile struct foo *meh = data;

	while ( true ){
		c4_msg_recieve( &msg, 0 );
		c4_msg_send( &msg, 2 );

		for ( unsigned i = 0; i < NUM_THREADS; i++ ){
			c4_msg_send( &msg, meh->threads[i] );
		}
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

int c4_create_thread( void (*entry)(void *), void *stack, void *data ){
	int ret = 0;

	DO_SYSCALL( SYSCALL_CREATE_THREAD, entry, stack, data, ret );

	return ret;
}
