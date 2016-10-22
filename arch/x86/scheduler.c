#include <c4/scheduler.h>

// small stub to save and restore current registers on thread switches
void sched_thread_yield( void ){
    asm volatile ( "pusha;"
                   "call sched_switch_thread;"
                   "popa;" );
}
