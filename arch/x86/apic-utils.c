#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <c4/arch/mp/pit.h>
#include <c4/arch/mp/mp.h>
#include <c4/arch/mp/apic.h>
#include <c4/arch/mp/ioapic.h>

#include <c4/klib/string.h>

#include <c4/paging.h>
#include <c4/debug.h>
#include <c4/arch/interrupts.h>
#include <c4/arch/paging.h>

void hexdump(void *addr, size_t n){
	uint8_t *ptr = addr;
	unsigned width  = 16;
	unsigned height = n / width;

	for (unsigned x = 0; x < height; x++){
		debug_printf("0x%x: ", ptr + x * width);

		for (unsigned y = 0; y < width; y++){
			uint8_t c = ptr[x * width + y];

			if (c < 0x10){
				debug_putchar('0');
			}

			debug_printf("%x ", c);
		}

		debug_printf("| ");

		for (unsigned y = 0; y < width; y++){
			uint8_t c = ptr[x * width + y];
			debug_putchar((c >= ' ' && c < 0x7f)? c : '.');
		}

		debug_printf("\n");
	}
}

void disable_pic(void){
	asm volatile ("mov $0xff, %al;"
				   "outb %al, $0xa1;"
				   "outb %al, $0x21;");
}

static uint32_t booted_cpus = 0;

void cpu_register(uint32_t cpu_num){
	booted_cpus |= 1 << cpu_num;
}

bool cpu_is_booted(uint32_t cpu_num){
	return (booted_cpus & (1 << cpu_num)) != 0;
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

/*
bool interrupt_callback(unsigned num, unsigned flags){
	debug_printf("LAPIC ID %u: interrupt %u\n", apic_get_id(), num);

	return false;
}
*/

void smp_thing(uint32_t cpu_num){
	asm volatile ("cli");
	debug_printf("- CPU %u is running\n", cpu_num);
	cpu_register(cpu_num);
	//addr_space_set(addr_space_kernel());

	apic_enable();
	init_cpu_segment_descs(cpu_num);
	init_cpu_interrupts();
	debug_printf("- CPU %u APIC is enabled\n", cpu_num);

	asm volatile ("sti");
	asm volatile ("int $1");
	asm volatile ("int $30");

	while (true) {
		asm volatile ("hlt");
	}
}

// TODO: rename and whatever
void foobizzbuzzlmao(void){
	debug_puts(">> Multiprocessor test\n");
	debug_puts(">> mapping io memory...\n");

	// iterate from the start of IO_PHYS_BASE to the end of memory
	// in 4MB increments mapping pages
	for (uintptr_t p = IO_PHYS_BASE; p; p += 0x400000) {
		debug_printf(">> %p => %p\n", p, io_phys_to_virt(p));
		map_phys_page_4mb(PAGE_READ | PAGE_WRITE | PAGE_SUPERVISOR,
						  (void *)io_phys_to_virt(p), (void *)p);
	}

	debug_puts("initializing asdf\n");
	disable_pic();
	smp_copy_boot_code();

	debug_puts(">> Initialized\n");

	if (!apic_supported()){
		debug_puts(">> CPU doesn't have an APIC lol\n");
		goto done;
	}

	apic_enable();

	mp_float_t *mp = mp_find();
	if (mp) {
		mp_enumerate(mp);

		/*
		debug_printf("trying bsp lapic at 0x%x:\n", lapic);
		//debug_printf(" - 0b%b\n", *(uint32_t *)lapic);
		hexdump(lapic, 128);
		debug_printf(" - id:      0x%x\n", apic_read(lapic, APIC_REG_ID));
		debug_printf(" - version: 0x%x\n", apic_read(lapic, APIC_REG_VERSION));

		debug_printf("trying ioapic at 0xfec00000\n");
		hexdump(ioapic, 128);
		*/

		void *ioapic = (void *)0xfec00000;
		debug_printf("- blarg: 0x%x\n", ioapic_read(ioapic, IOAPIC_REG_VERSION));

	} else {
		debug_puts(">> Did not find MP tables\n");
	}

done:
	debug_puts(">> CPU initialization done\n");
	apic_timer();

	/*
	asm volatile ("sti");
	asm volatile ("int $3");
	apic_timer();

	while (true) {
		asm volatile ("hlt");
	}
	*/
}
