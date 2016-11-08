#include <c4/arch/thread.h>
#include <c4/thread.h>
#include <c4/klib/string.h>
#include <c4/mm/region.h>
#include <c4/scheduler.h>

void thread_set_init_state( thread_t *thread,
                            void (*entry)(void *data),
                            void *data,
                            void *stack,
                            unsigned flags )
{
	memset( thread, 0, sizeof( thread_t ));

	uint32_t *new_stack = stack;

	// argument for entry function
	*(--new_stack) = (uint32_t)data;

	// return address, set to the exit function
	// so threads exit when they return
	*(--new_stack) = (uint32_t)sched_thread_exit;

	thread->stack         = new_stack;
	thread->registers.esp = (uint32_t)new_stack;
	thread->registers.eip = (uint32_t)entry;
	thread->registers.do_user_switch = !!(flags & THREAD_FLAG_USER);
}
