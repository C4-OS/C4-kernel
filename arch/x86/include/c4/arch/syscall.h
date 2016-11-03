#ifndef _C4_ARCH_SYSCALL_H
#define _C4_ARCH_SYSCALL_H 1
#include <c4/arch/interrupts.h>

void syscall_handler( interrupt_frame_t *frame );

#endif
