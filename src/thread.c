#include <c4/thread.h>
#include <c4/mm/slab.h>
#include <c4/mm/region.h>
#include <c4/common.h>

static slab_t thread_slab;
static unsigned thread_counter = 0;

void init_threading( void ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &thread_slab, region_get_global(),
	  	              sizeof(thread_t), NULL, NULL );

		initialized = true;
	}
}

thread_t *thread_create( void (*entry)(void *data), void *data ){
	thread_t *ret = slab_alloc( &thread_slab );

	ret->thread_id = ++thread_counter;
	ret->task_id   = 1;

	return ret;
}

void thread_destroy( thread_t *thread ){
	slab_free( &thread_slab, thread );
}

// TODO: maybe merge the slab list functions with the functions here
//       using a common doubly-linked list implementation
void thread_list_insert( thread_list_t *list, thread_t *thread ){
	thread->list = list;
	thread->next = list->first;
	thread->prev = NULL;

	if ( list->first ){
		list->first->prev = thread;
	}

	list->first = thread;
}

void thread_list_remove( thread_t *thread ){
	if ( thread->list ){
		if ( thread->prev ){
			thread->prev->next = thread->next;
		}

		if ( thread->next ){
			thread->next->prev = thread->prev;
		}

		if ( thread == thread->list->first ){
			thread->list->first = thread->next;
		}
	}
}

thread_t *thread_list_pop( thread_list_t *list ){
	thread_t *ret = NULL;

	if ( list->first ){
		ret = list->first;
		thread_list_remove( list->first );
	}

	return ret;
}

thread_t *thread_list_peek( thread_list_t *list ){
	thread_t *ret = NULL;

	if ( list->first ){
		ret = list->first;
	}

	return ret;
}
