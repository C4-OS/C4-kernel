#include <c4/debug.h>
#include <c4/arch/mp/apic.h>
#include <c4/arch/mp/cpu.h>
#include <c4/arch/paging.h>

#include <stdint.h>
#include <stdbool.h>

uint32_t apic_read(void *addr, uint32_t reg){
	uint32_t volatile *apic = addr;

	return *(apic + (reg / 4));
}

void apic_write(void *addr, uint32_t reg, uint32_t value){
	uint32_t volatile *apic = addr;

	*(apic + (reg / 4)) = value;
}

void apic_send_ipi(void *addr, uint32_t type, uint8_t lapic_id){
	//uint32_t volatile *apic = addr;
	//uint32_t x = apic_read(addr, APIC_REG_INTR_COMMAND_LOWER);
	uint32_t x = 0;

	//x &= ~0x7000000;
	x |= lapic_id << 24;

	apic_write(addr, APIC_REG_INTR_COMMAND_LOWER, x);
	apic_write(addr, APIC_REG_INTR_COMMAND_UPPER, type);
}

bool apic_ipi_recieved(void *apic){
	uint32_t status = apic_read(apic, APIC_REG_INTR_COMMAND_LOWER);

	return (status & APIC_ICR_DELIVERY_STATUS) == 0;
}

bool apic_supported(void){
	uint32_t a, d;

	cpuid(CPUID_GET_FEATURES, &a, &d);

	return d & CPUID_FEATURE_APIC;
}

void apic_set_base(uintptr_t apic){
	apic = io_virt_to_phys(apic);

	uint32_t edx = 0;
	uint32_t eax = (apic & ~0xfff) | IA32_MSR_APIC_BASE_ENABLE;

	cpu_write_msr(IA32_MSR_APIC_BASE, eax, edx);
}

uintptr_t apic_get_base(void){
	uint32_t eax, edx;

	cpu_read_msr(IA32_MSR_APIC_BASE, &eax, &edx);

	return io_phys_to_virt(eax & ~0xfff);
}

void apic_enable(void){
	uintptr_t apic = apic_get_base();
	apic_set_base(apic);

	debug_printf(" - apic base is %p\n", apic);

	// TODO: APIC base address will need to be mapped into virtual memory
	//       when paging is enabled, maybe have 0xfee00000 -> 0xfffff000
	//       be identity-mapped
	uint32_t spiv = apic_read((void *)apic, APIC_REG_SPURIOUS_INTR_VEC);
	apic_write((void *)apic, APIC_REG_SPURIOUS_INTR_VEC, spiv | 0x100);
}

uint32_t apic_version(void){
	void *apic = (void *)apic_get_base();

	return apic_read(apic, APIC_REG_VERSION);
}

uint32_t apic_get_id(void){
	void *apic = (void *)apic_get_base();

	return apic_read(apic, APIC_REG_ID) >> 24;
}

enum {
	APIC_NMI = (4 << 8),
	TMR_PERIODIC = 0x20000,
	TMR_BASEDIV = (1 << 20),
};

void apic_timer(void){
	void *apic = (void *)apic_get_base();

	apic_write(apic, APIC_REG_LOCAL_TIMER, 32 | TMR_PERIODIC);
	apic_write(apic, APIC_REG_TIMER_DIV_CONF, 0x3);
	apic_write(apic, APIC_REG_TIMER_INIT_COUNT, 0x100);
}
