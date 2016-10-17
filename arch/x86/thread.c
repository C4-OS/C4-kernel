#include <c4/arch/thread.h>

void thread_store_registers( thread_regs_t *buf );
void thread_set_init_state( thread_t *thread, void (*entry)(void *data) );
