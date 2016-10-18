#ifndef _C4_THREAD_H
#define _C4_THREAD_H 1
#include <c4/arch/thread.h>
#include <c4/paging.h>
#include <c4/message.h>

typedef struct thread thread_t;

typedef struct thread_list {
	thread_t *first;
	unsigned size;
} thread_list_t;

typedef struct thread {
	page_dir_t    *page_dir;
	thread_t      *next;
	thread_t      *prev;
	thread_list_t *list;
	void          *stack;

	thread_regs_t registers;
	message_t     message;

	unsigned id;
	unsigned task_id;
	unsigned priority;
	unsigned state;
	unsigned flags;
} thread_t;

void init_threading( void );
thread_t *thread_create( void (*entry)(void *data), void *data );
void thread_destroy( thread_t *thread );

void thread_list_insert( thread_list_t *list, thread_t *thread );
void thread_list_remove( thread_t *thread );
thread_t *thread_list_pop( thread_list_t *list );
thread_t *thread_list_peek( thread_list_t *list );

#endif
