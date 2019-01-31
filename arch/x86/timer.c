#include <c4/timer.h>
#include <c4/scheduler.h>
#include <c4/debug.h>

// TODO: architecture-specific
#include <c4/arch/pit.h>

// which CPU was the bootstrapper
// TODO: should this be somewhere else?
static unsigned starting_cpu;
// TODO: measure each core
static uint64_t cpu_frequency;

// TODO: architecture-specific
uint64_t timer_get_timestamp(void){
	register unsigned a, d;

	asm volatile ( "rdtsc" : "=a"(a), "=d"(d));

	return ((uint64_t)d << 32) | a;
}

// TODO: architecture-specific
static uint64_t measure_cpu_freq(void) {
	uint64_t start = timer_get_timestamp();
	pit_wait_poll(PIT_WAIT_10MS);
	uint64_t end = timer_get_timestamp();

	return (end - start) * 100;
}

uint64_t timer_get_timestamp_ns(void) {
	return (timer_get_timestamp() * 1000000) / (cpu_frequency / 1000);
}

void timers_init(void) {
	// TODO: check for constant tsc bit, and fall back to apic/hpet if it's
	//       not available
	starting_cpu = sched_current_cpu();
	cpu_frequency = measure_cpu_freq();

	debug_printf("CPU frequency: %uMHz\n", (uint32_t)(cpu_frequency / 1000000));
}

uint64_t timer_uptime(void) {
	// XXX: assumes timestamps are synced between CPUs
	int64_t uptime = timer_get_timestamp();

	KASSERT(uptime > 0);
	return (uptime * 1000000) / (cpu_frequency / 1000);
}
