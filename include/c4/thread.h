#ifndef _C4_THREAD_H
#define _C4_THREAD_H 1
#include <c4/arch/thread.h>
#include <c4/paging.h>

typedef struct thread {
	unsigned thread_id;
	unsigned task_id;

	thread_regs_t registers;
	page_dir_t    *page_dir;

	struct thread *next;
	struct thread *prev;
} thread_t;

unsigned thread_spawn( void (*entry)(void *data) );
unsigned thread_spawn_at( thread_t *thread, void (*entry)(void *data) );
void thread_exit( void );

#endif
