#include <c4/arch/thread.h>
#include <c4/thread.h>
#include <c4/debug.h>
#include <c4/common.h>
#include <c4/klib/string.h>
#include <c4/mm/region.h>
#include <c4/scheduler.h>

void thread_set_init_state( thread_t *thread,
                            void (*entry)(void),
                            void *stack,
                            unsigned flags )
{
	memset( thread, 0, sizeof( thread_t ));

	uint8_t *kern_stack = region_alloc( region_get_global( ));

	KASSERT( kern_stack != NULL );

	thread->kernel_stack  = kern_stack + PAGE_SIZE;
	thread->registers.esp = (uint32_t)stack;
	thread->registers.eip = (uint32_t)entry;
	thread->registers.do_user_switch = !!(flags & THREAD_FLAG_USER);
}
