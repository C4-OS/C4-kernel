#include <c4/thread.h>
#include <c4/mm/slab.h>
#include <c4/mm/region.h>
#include <c4/common.h>
#include <c4/debug.h>

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

thread_t *thread_create( void (*entry)(void),
                         addr_space_t *space,
                         void *stack,
                         unsigned flags )
{
	thread_t *ret = slab_alloc(&thread_slab);

	thread_set_init_state(ret, entry, stack, flags);

	ret->id         = thread_counter++;
	ret->addr_space = space;
	ret->flags      = flags;
	ret->cap_space  = NULL;

	// TODO: register with kobject list
	//thread_list_insert(&thread_global_list, &ret->intern);

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

void thread_destroy(thread_t *thread) {
	//thread_list_remove( &thread->intern );
	slab_free(&thread_slab, thread);
}

void thread_set_addrspace( thread_t *thread, addr_space_t *aspace ){
	// TODO: locking/atomic set
	thread->addr_space = aspace;
}

void thread_set_capspace( thread_t *thread, cap_space_t *cspace ){
	// TODO: locking/atomic set
	thread->cap_space = cspace;
}
