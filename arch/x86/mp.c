#include <c4/debug.h>
#include <c4/arch/paging.h>

#include <c4/arch/mp.h>
#include <c4/arch/apic.h>
#include <c4/arch/ioapic.h>
#include <c4/arch/cmos.h>
#include <c4/arch/pit.h>
#include <c4/arch/bios.h>

#include <stdbool.h>
#include <stdint.h>

void cpu_register(uint32_t cpu_num);
bool cpu_is_booted(uint32_t cpu_num);

// TODO: generic CPU info struct that doesn't rely on mp table entries,
//       so that this function can be used with both IMPS and ACPI-based
//       machines
void mp_boot_cpu(void *lapic, mp_cpu_entry_t *cpu, unsigned cpu_num){
	debug_puts(">> trying to boot CPU ");
	debug_putchar('0' + cpu_num);
	debug_puts("\n");

	if (cpu->flags & MP_CPU_FLAG_BOOTSTRAPPER){
		// this is the bootstrapping CPU, nothing to do here!
		cpu_register(cpu_num);
		debug_puts(">> bootstrapping CPU is already running\n");
		debug_printf(" - er, nevermind\n");
		return;
	}

	uint32_t bootaddr = 0xa000;

	//smp_copy_boot_code();

	cmos_write(CMOS_REG_RESET, CMOS_RESET_JUMP);
	*((volatile uint32_t *)low_phys_to_virt(BIOS_RESET_VECTOR)) = (bootaddr & 0xff000) << 12;

	debug_printf(" - cpu: %p, %p, apic: %p\n", cpu, cpu->local_apic_id, lapic);

	debug_printf(" - sending INIT\n");
	apic_send_ipi(lapic,
				   /* APIC_ICR_DM | */ APIC_ICR_LEVEL |
				   APIC_ICR_LEVEL_ASSERT | APIC_IPI_INIT,
				   cpu->local_apic_id);
	debug_printf(" - waiting...\n");
	pit_wait_poll(PIT_WAIT_10MS);

	debug_printf(" - deasserting INIT...\n");
	apic_send_ipi(lapic,
				   /*APIC_ICR_DM |*/ APIC_ICR_LEVEL | APIC_IPI_INIT,
				   cpu->local_apic_id);
	pit_wait_poll(PIT_WAIT_10MS);

	if (!apic_ipi_recieved(lapic)){
		debug_printf(" - didn't seem to get the INIT IPIs...\n");
	}

	debug_printf(" - sending START...\n");
	apic_send_ipi(lapic, /* APIC_ICR_DM | */ APIC_IPI_START | ((bootaddr >> 12)& 0xff), cpu->local_apic_id);
	pit_wait_poll(PIT_WAIT_10MS * 3);

	if (!cpu_is_booted(cpu_num)){
		debug_printf(" - didn't start, trying again...\n");
		apic_send_ipi(lapic, APIC_IPI_START | ((bootaddr >> 12)& 0xff), cpu->local_apic_id);
		pit_wait_poll(PIT_WAIT_10MS);
	}

	if (!apic_ipi_recieved(lapic)){
		debug_printf(" - second START IPI wasn't recieved.\n");
	}

	if (cpu_is_booted(cpu_num)){
		debug_printf(" - CPU %u booted successfully\n", cpu_num);

	} else {
		debug_printf(" - Couldn't boot CPU %u, continuing...\n", cpu_num);
	}

	cmos_write(CMOS_REG_RESET, CMOS_RESET_POWERON);
	*((volatile uint32_t *)low_phys_to_virt(BIOS_RESET_VECTOR))= (bootaddr & 0xff000)<< 12;
	//*((volatile uint32_t *)BIOS_RESET_VECTOR)= 0;
}

void mp_handle_cpu(void *lapic, void *ptr){
	static unsigned cpu_count = 0;
	mp_cpu_entry_t *entry = ptr;

	debug_printf(" - lapic id: %u\n", entry->local_apic_id);
	debug_printf(" - flags:    0x%x ", entry->flags);

	if (entry->flags & 0x1)debug_puts("(present)");
	if (entry->flags & 0x2)debug_puts("(bootstrapper)");

	debug_putchar('\n');
	debug_printf(" - trying to boot...\n");

	mp_boot_cpu(lapic, entry, cpu_count++);
}

void mp_handle_bus(void *lapic, void *ptr){
	mp_bus_t *bus = ptr;

	debug_printf(" - id:       %u\n", bus->id);
	debug_printf(" - type:     ");

	for (unsigned x = 0; x < 6; x++)
		debug_putchar(bus->bus_type[x]);

	debug_putchar('\n');
}

// TODO: probably want to have a generic function to initialize the ioapic
//       and interrupts and whatever, because we'll have to implement ACPI at
//       some point too. This can wait until actually doing ACPI stuff though.
void mp_handle_ioapic(void *lapic, void *ptr){
	mp_io_apic_t *ioapic = ptr;

	debug_printf(" - id:       %u\n",   ioapic->id);
	debug_printf(" - address:  0x%x\n", ioapic->address);

	void *ioapic_ptr = io_phys_to_virt(ioapic->address);
	ioapic_add(ioapic_ptr, ioapic->id);
	ioapic_print_redirects(ioapic->id);

	/*
	// both the version and maximum redirects are encoded in the version register
	uint32_t version_reg = ioapic_read(ioapic_ptr, IOAPIC_REG_VERSION);
	uint32_t version = version_reg & 0xff;
	uint8_t  max_redirects = (version_reg >> 16) & 0xff;

	uint32_t redirect     = ioapic_read(ioapic_ptr, IOAPIC_REG_REDIRECT);
	uint32_t redirect_end = ioapic_read(ioapic_ptr, IOAPIC_REG_REDIRECT_END);

	debug_printf(" - version:  0x%x\n", version);
	debug_printf(" - max red:  0x%x\n", max_redirects);
	debug_printf(" - redirect: 0x%x\n", redirect);
	debug_printf(" - red end:  0x%x\n", redirect_end);

	debug_printf(" - redirection entries:\n");

	for (unsigned i = 0; i < max_redirects; i++) {
		uint32_t index = IOAPIC_REG_REDIRECT + (i * 2);
		uint32_t lower = ioapic_read(ioapic_ptr, index);
		uint32_t upper = ioapic_read(ioapic_ptr, index + 1);

		debug_printf("    - %x:%x\n", upper, lower);
		debug_printf("    - vector: 0x%x\n", lower & 0xf);
		debug_printf("    - delivery: 0x%x\n", (lower >> 8) & 0x3);
		debug_printf("    - dest. mode: 0x%x\n", (lower >> 11) & 0x1);
		debug_printf("    - status: 0x%x\n", (lower >> 12) & 0x1);
		debug_printf("    - polarity: 0x%x\n", (lower >> 13) & 0x1);
		debug_printf("    - remote irr: 0x%x\n", (lower >> 14) & 0x1);
		debug_printf("    - trigger: 0x%x\n", (lower >> 15) & 0x1);
		debug_printf("    - mask: 0x%x\n", (lower >> 16) & 0x1);
		debug_printf("    - destination: 0x%x\n", upper >> 24);
	}
	*/
}

void mp_handle_io_interrupt(void *lapic, void *ptr){
	mp_interrupt_t *intr = ptr;

	debug_printf(" - int type: %u\n", intr->int_type);
	debug_printf(" - src id:   %u\n", intr->source_bus_id);
	debug_printf(" - src irq:  %u\n", intr->source_bus_irq);
	debug_printf(" - dst id:   %u\n", intr->dest_apic_id);
	debug_printf(" - dst irq:  %u\n", intr->dest_apic_intin);

	ioapic_redirect_t redirect = ioapic_get_redirect(intr->dest_apic_id,
	                                                 intr->dest_apic_intin);

	uint8_t active_high = (intr->int_type & 3) == 1;
	uint8_t level_triggered = ((intr->int_type >> 2) & 3) == 3;

	debug_printf(" - act. hi:  %u\n", active_high);
	debug_printf(" - lvl trig: %u\n", level_triggered);

	redirect.vector = intr->dest_apic_intin + 32; // IRQs start at 32
	redirect.delivery = IOAPIC_DELIVERY_FIXED;
	redirect.destination_mode = IOAPIC_DESTINATION_PHYSICAL;
	redirect.polarity = !active_high;
	redirect.trigger = level_triggered;
	redirect.mask = false;
	redirect.destination = 0; /* TODO: would it be better to
	                                   assign to other CPUs? */

	ioapic_set_redirect(intr->dest_apic_id, intr->dest_apic_intin, &redirect);
}

void mp_handle_interrupt(void *lapic, void *ptr){
	mp_interrupt_t *intr = ptr;

	debug_printf(" - int type: %u\n", intr->int_type);
	debug_printf(" - src id:   %u\n", intr->source_bus_id);
	debug_printf(" - src irq:  %u\n", intr->source_bus_irq);
	debug_printf(" - dst id:   %u\n", intr->dest_apic_id);
	debug_printf(" - dst irq:  %u\n", intr->dest_apic_intin);
}

mp_float_t *mp_find(void){
	void *bios_ebda_rom = (void *)low_phys_to_virt(0xf0000);

	return find_bios_key("_MP_", bios_ebda_rom, 1024 * 64);
}

void mp_enumerate(mp_float_t *mp){
	if (!mp){
		return;
	}

	if (!apic_supported()){
		debug_puts(">> CPU doesn't have an APIC lol\n");
		debug_puts(">> CPU doesn't have an APIC lol\n");
		return;
	}

	debug_puts(">> Found MP tables, trying to start APs...\n");
	//hexdump(mp, 512);

	debug_printf("found multiprocessing struct: %p\n", mp);
	debug_printf(" - length:   %u\n", mp->length);
	debug_printf(" - revision: %u\n", mp->revision);
	debug_printf(" - features: %b\n", mp->features);
	debug_printf(" - config:   0x%x\n", mp->config_table);

	mp_config_t *config = (void *)low_phys_to_virt(mp->config_table);

	debug_printf("found mp config table: 0x%x\n", config);
	debug_printf(" - length:   %u\n", config->length);
	debug_printf(" - entries:  %u\n", config->entry_count);
	debug_printf(" - lapic:    0x%x\n", config->lapic_addr);

	void *lapic = (void *)io_phys_to_virt(config->lapic_addr);
	debug_printf(" - adjusted: %p\n", lapic);

	mp_cpu_entry_t *entry = (void *)((uint8_t *)config + sizeof(mp_config_t));

	for (unsigned i = 0; i < config->entry_count; i++){
		debug_printf("found mp table entry: 0x%x\n", entry);
		bool is_cpu = entry->type == 0;
		char *types[] = { "cpu", "bus", "ioapic", "interrupt", "interrupt" };

		debug_printf(" - type:     %u (%s)\n",
					  entry->type,
					  types[entry->type]);

		switch (entry->type){
			case MP_TYPE_CPU:
				mp_handle_cpu(lapic, entry);
				break;

			case MP_TYPE_BUS:
				mp_handle_bus(lapic, entry);
				break;

			case MP_TYPE_IOAPIC:
				mp_handle_ioapic(lapic, entry);
				break;

			case MP_TYPE_INTERRUPT:
				mp_handle_io_interrupt(lapic, entry);
				break;

			case MP_TYPE_INTERRUPT_A:
				mp_handle_interrupt(lapic, entry);
				break;

			default: break;
		}

		entry = (void *)((uint8_t *)entry + (is_cpu? 20 : 8));
	}
}
