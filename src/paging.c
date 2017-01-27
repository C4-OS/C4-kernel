#include <c4/paging.h>
#include <c4/message.h>
#include <c4/scheduler.h>
#include <c4/common.h>
#include <c4/debug.h>

void page_fault_message( uintptr_t address ){
	thread_t *cur = sched_current_thread( );

	if ( !cur ){
		// TODO: panic
		debug_printf( "error: page fault before scheduler has threads!\n" );
		for (;;);
	}

	message_t faultmsg = {
		.type = MESSAGE_TYPE_PAGE_FAULT,
		.data = { address, }
	};

	debug_printf( "=== sending a page fault message to %u, address: %p\n",
		cur->pager, address );

	SET_FLAG( cur, THREAD_FLAG_FAULTED );
	message_send( &faultmsg, cur->pager );
	sched_thread_yield( );
}
