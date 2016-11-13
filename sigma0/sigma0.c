#include <sigma0/sigma0.h>

void test_thread( void *unused );

/*
int c4_msg_send( message_t *buffer, unsigned target );
int c4_msg_recieve( message_t *buffer, unsigned whom );
int c4_create_thread( void (*entry)(void *), void *stack, void *data );
*/

#define NUM_THREADS 10

struct foo {
	int target;
	int threads[NUM_THREADS];
	int display;
};

void main( void ){
	unsigned *s = (void *)0xbfffe000;
	struct foo thing;
	message_t start = (message_t){ .type = MESSAGE_TYPE_CONTINUE, };

	thing.target  = 2;
	thing.display = c4_create_thread( display_thread, s, NULL );
	c4_msg_send( &start, thing.display );
	s -= 1024;

	for ( unsigned i = 0; i < NUM_THREADS; i++ ){
		thing.threads[i] = c4_create_thread( test_thread, s, &thing );
		s -= 1024;

		c4_msg_send( &start, thing.threads[i] );
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

void display_prompt( struct foo *thing ){
	message_t msg;
	char *prompt = "asdf foo bar baz > ";

	for ( unsigned i = 0; prompt[i]; i++ ){
		msg.data[0] = prompt[i];
		msg.type    = 0xbabe;

		c4_msg_send( &msg, thing->display );
	}
}

void server( void *data ){
	message_t msg;
	struct foo *meh = data;
	bool stopped = false;
	bool do_send = false;

	display_prompt( meh );

	while ( true ){
		c4_msg_recieve( &msg, 0 );
		//c4_msg_send( &msg, 2 );

		if ( msg.data[1] == 0 ){
			c4_msg_send( &msg, meh->display );

			if ( msg.data[0] == 28 ){
				display_prompt( meh );
			}
		}

		do_send = false;

		if ( !stopped && msg.data[0] == 31 /* 's' */ ) {
			msg.type = MESSAGE_TYPE_STOP;
			stopped = true;
			do_send = true;

		} else if ( stopped && msg.data[0] == 46 /* 'c' */ ){
			msg.type = MESSAGE_TYPE_CONTINUE;
			stopped = false;
			do_send = true;

		} else if ( !stopped ){
			do_send = true;
		}

		if ( do_send ){
			for ( unsigned i = 0; i < NUM_THREADS; i++ ){
				c4_msg_send( &msg, meh->threads[i] );
			}
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
