#ifndef _MP_CPU_H
#define _MP_CPU_H 1

enum {
	CPUID_GET_FEATURES = 1,
};

enum {
	CPUID_FEATURE_APIC = 1 << 9,
};

enum {
	IA32_MSR_APIC_BASE        = 0x1b,
	IA32_MSR_APIC_BASE_BSP    = 1 << 8,
	IA32_MSR_APIC_BASE_ENABLE = 1 << 11,
};

static inline void cpuid( uint32_t code, uint32_t *eax, uint32_t *edx ){
	asm volatile( "cpuid"
	              : "=a"(*eax), "=d"(*edx)
	              : "a"(code)
	              : "ecx", "ebx" );
}

static inline void cpu_read_msr( uint32_t msr, uint32_t *low, uint32_t *high ){
	asm volatile( "rdmsr"
	              : "=a"(*low), "=d"(*high)
	              : "c"(msr) );
}

static inline void cpu_write_msr( uint32_t msr, uint32_t low, uint32_t high ){
	asm volatile( "wrmsr"
	              :
	              : "a"(low), "d"(high), "c"(msr) );
}

#endif
