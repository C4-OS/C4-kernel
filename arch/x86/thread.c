#include <c4/arch/thread.h>
#include <c4/thread.h>
#include <c4/klib/string.h>
#include <c4/mm/region.h>
#include <c4/scheduler.h>

void thread_set_init_state( thread_t *thread,
                            void (*entry)(void *data),
                            void *data )
{
	memset( thread, 0, sizeof( thread_t ));

	uint32_t *stack = region_alloc( region_get_global() );
	unsigned ptr    = PAGE_SIZE / sizeof(*stack);

	// argument for entry function
	stack[--ptr] = (uint32_t)data;

	// return address, set to the exit function
	// so threads exit when they return
	stack[--ptr] = (uint32_t)sched_thread_exit;

	thread->stack         = stack;
	thread->registers.esp = (uint32_t)(stack + ptr);
	thread->registers.eip = (uint32_t)entry;
}
