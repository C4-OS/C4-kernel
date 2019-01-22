#ifndef _C4_THREAD_H
#define _C4_THREAD_H 1
#include <c4/arch/thread.h>
#include <c4/klib/queue.h>
#include <c4/mm/addrspace.h>
#include <c4/paging.h>
#include <c4/capability.h>

enum {
	THREAD_FLAG_NONE        = 0,
	THREAD_FLAG_USER        = 1,
	THREAD_FLAG_ROOT_TASK   = 2,
	THREAD_FLAG_PENDING_MSG = 4,
	THREAD_FLAG_FAULTED     = 8,
};

enum {
	THREAD_CREATE_FLAG_NONE   = 0,
	THREAD_CREATE_FLAG_CLONE  = 1,
	THREAD_CREATE_FLAG_NEWMAP = 2,
};

typedef struct thread      thread_t;
typedef struct thread_node thread_node_t;

TYPESAFE_QUEUE(thread_t, thread_queue);

#include <c4/message.h>

typedef struct thread {
	thread_regs_t registers;
	void          *kernel_stack;

	addr_space_t  *addr_space;
	cap_space_t   *cap_space;
	// pointer to IPC endpoint which is used to communicate with the pager
	// TODO: hmm, objects in capabilities are going to need their own
	//       reference counting, since storing the capability entry itself
	//       here isn't an option. If the capability entry changes, then the
	//       object which is pointed to might be a different endpoint, or
	//       maybe not even an endpoint at all. Definitely a security issue.
	msg_queue_t   *pager;

	unsigned id;
	unsigned state;
	unsigned flags;

	message_t message;
} thread_t;

void init_threading( void );
thread_t *thread_create( void (*entry)(void),
                         addr_space_t *space,
                         void *stack,
                         unsigned flags );

thread_t *thread_create_kthread( void (*entry)(void));

void thread_destroy( thread_t *thread );
void thread_set_addrspace( thread_t *thread, addr_space_t *aspace );
void thread_set_capspace( thread_t *thread, cap_space_t *cspace );

// functions below are implemented in arch-specific code
void thread_set_init_state( thread_t *thread,
                            void (*entry)(void),
                            void *stack,
                            unsigned flags );

void usermode_jump( void *entry, void *stack );

#endif
