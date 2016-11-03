#include <c4/arch/syscall.h>
#include <c4/syscall.h>
#include <c4/debug.h>

void syscall_handler( interrupt_frame_t *frame ){
	unsigned num = frame->eax;

	frame->eax = syscall_dispatch( num, frame->edi, frame->esi, frame->edx );
}
