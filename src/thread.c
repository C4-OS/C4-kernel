#include <c4/thread.h>
#include <c4/mm/slab.h>
#include <c4/mm/region.h>
#include <c4/common.h>
#include <c4/debug.h>

static slab_t thread_slab;
static unsigned thread_counter = 0;
thread_list_t thread_global_list = {
	.first = NULL,
	.size  = 0,
};

void init_threading( void ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &thread_slab, region_get_global(),
	  	              sizeof(thread_t), NULL, NULL );

		initialized = true;
	}
}

thread_t *thread_create( void (*entry)(void),
                         addr_space_t *space,
                         void *stack,
                         unsigned flags )
{
	thread_t *ret = slab_alloc( &thread_slab );

	thread_set_init_state( ret, entry, stack, flags );

	ret->sched.thread  = ret;
	ret->intern.thread = ret;

	ret->id         = thread_counter++;
	ret->task_id    = 1;
	ret->addr_space = space;
	ret->flags      = flags;

	thread_list_insert( &thread_global_list, &ret->intern );

	return ret;
}

thread_t *thread_create_kthread( void (*entry)(void)){
	uint8_t *stack = region_alloc( region_get_global( ));

	KASSERT( stack != NULL );
	stack += PAGE_SIZE;

	return thread_create( entry,
	                      addr_space_reference( addr_space_kernel( )),
	                      stack,
	                      THREAD_FLAG_NONE );
}

void thread_destroy( thread_t *thread ){
	thread_list_remove( &thread->intern );
	slab_free( &thread_slab, thread );
}

// TODO: maybe merge the slab list functions with the functions here
//       using a common doubly-linked list implementation
void thread_list_insert( thread_list_t *list, thread_node_t *node ){
	node->list = list;
	node->next = list->first;
	node->prev = NULL;

	if ( list->first ){
		list->first->prev = node;
	}

	list->first = node;
}

void thread_list_remove( thread_node_t *node ){
	if ( node->list ){
		if ( node->prev ){
			node->prev->next = node->next;
		}

		if ( node->next ){
			node->next->prev = node->prev;
		}

		if ( node == node->list->first ){
			node->list->first = node->next;
		}
	}
}

thread_t *thread_list_pop( thread_list_t *list ){
	thread_t *ret = NULL;

	if ( list->first ){
		ret = list->first->thread;
		thread_list_remove( list->first );
	}

	return ret;
}

thread_t *thread_list_peek( thread_list_t *list ){
	thread_t *ret = NULL;

	if ( list->first ){
		ret = list->first->thread;
	}

	return ret;
}

thread_t *thread_get_id( unsigned id ){
	thread_node_t *node = thread_global_list.first;

	for ( ; node; node = node->next ){
		if ( node->thread->id == id ){
			return node->thread;
		}
	}

	return NULL;
}
