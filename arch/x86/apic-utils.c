#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <c4/arch/pit.h>
#include <c4/arch/mp.h>
#include <c4/arch/apic.h>
#include <c4/arch/ioapic.h>
#include <c4/arch/pic.h>

#include <c4/klib/string.h>

#include <c4/arch/interrupts.h>
#include <c4/arch/paging.h>
#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/scheduler.h>

static uint32_t booted_cpus = 0;
static unsigned num_booted_cpus = 0;

void cpu_register(uint32_t cpu_num){
	booted_cpus |= 1 << cpu_num;
	num_booted_cpus++;
}

bool cpu_is_booted(uint32_t cpu_num){
	return (booted_cpus & (1 << cpu_num)) != 0;
}

unsigned sched_num_cpus(void) {
	return num_booted_cpus;
}

unsigned sched_current_cpu(void) {
	if (sched_num_cpus() > 1) {
		return apic_get_id();

	} else {
		return 0;
	}
}

void smp_copy_boot_code(void){
	uint32_t bootaddr = low_phys_to_virt(0xa000);
	extern void *smp_boot_start;
	extern void *smp_boot_end;

	size_t smp_code_size = (uintptr_t)&smp_boot_end - (uintptr_t)&smp_boot_start;

	debug_printf(" - copying %u bytes of patch code, %p -> %p\n",
	              smp_code_size, &smp_boot_start, bootaddr);

	memcpy((void *)bootaddr, &smp_boot_start, smp_code_size);
}

void smp_thing(uint32_t cpu_num){
	asm volatile ("cli");
	debug_printf("- CPU %u is running\n", cpu_num);
	cpu_register(cpu_num);
	//addr_space_set(addr_space_kernel());

	apic_enable();
	init_cpu_segment_descs(cpu_num);
	init_cpu_interrupts();
	sched_init_cpu();
	debug_printf("- CPU %u APIC is enabled\n", cpu_num);
	debug_printf("- CPU %u APIC TPR: %u\n",
	             cpu_num, apic_read(apic_get_base(), APIC_REG_TASK_PRIORITY));

	// TODO: initialize CPU thread context here

	// set up timer, so the global timer handler gets called
	apic_timer_one_shot(0x8000000);

	while (true) {
		asm volatile ("sti");
		asm volatile ("hlt");
	}
}

// TODO: rename and whatever
void smp_init(void){
	debug_puts(">> Multiprocessor test\n");
	debug_puts(">> mapping io memory...\n");

	// iterate from the start of IO_PHYS_BASE to the end of memory
	// in 4MB increments mapping pages
	for (uintptr_t p = IO_PHYS_BASE; p; p += 0x400000) {
		debug_printf(">> %p => %p\n", p, io_phys_to_virt(p));
		map_phys_page_4mb(PAGE_READ | PAGE_WRITE | PAGE_SUPERVISOR,
						  (void *)io_phys_to_virt(p), (void *)p);
	}

	if (!apic_supported()){
		debug_puts(">> CPU doesn't have an APIC lol\n");
		goto done;
	}

	// XXX: need this here so the number of apic timer ticks can be cached
	//      in apic_timer_usec_to_ticks()
	uint32_t ticks = apic_timer_usec_to_ticks(1000000);
	debug_printf(">> APIC ticks in 1s: %u\n", ticks);

	debug_puts("initializing asdf\n");
	disable_pic();
	smp_copy_boot_code();
	debug_puts(">> Initialized\n");
	apic_enable();

	mp_float_t *mp = mp_find();
	if (mp) {
		mp_enumerate(mp);

	} else {
		debug_puts(">> Did not find MP tables\n");
	}

done:
	debug_puts(">> CPU initialization done\n");
	apic_timer_one_shot(0x800000);
}
