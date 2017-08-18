#include <c4/paging.h>
#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/common.h>
#include <c4/debug.h>

void page_fault_message( uintptr_t address, uintptr_t ip, unsigned perms ){
	thread_t *cur = sched_current_thread( );

	if ( !cur ){
		// TODO: panic
		debug_printf( "error: page fault before scheduler has threads!\n" );
		for (;;);
	}

	message_t faultmsg = {
		.type = MESSAGE_TYPE_PAGE_FAULT,
		.data = { address, ip, perms },
	};

	debug_printf( "=== sending a page fault message to %p, address: %p\n",
		cur->pager, address );

	SET_FLAG( cur, THREAD_FLAG_FAULTED );
	sched_thread_stop( cur );

	if ( cur->pager ){
		message_send( cur->pager, &faultmsg );
	} else {
		debug_printf( "error: faulted thread %u has no pager!\n", cur->id );
	}

	sched_thread_yield( );
}
