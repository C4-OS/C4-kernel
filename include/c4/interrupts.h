#ifndef _C4_INTERRUPTS_H
#define _C4_INTERRUPTS_H 1
#include <c4/thread.h>
#include <stdint.h>
#include <stdbool.h>

bool interrupt_callback( unsigned num, unsigned flags );
int  interrupt_listen( unsigned num, thread_t *thread );

#endif
