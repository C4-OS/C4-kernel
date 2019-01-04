#ifndef _MP_MP_H
#define _MP_MP_H 1
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum {
	MP_TYPE_CPU,
	MP_TYPE_BUS,
	MP_TYPE_IOAPIC,
	MP_TYPE_INTERRUPT,
	MP_TYPE_INTERRUPT_A,
};

enum {
	MP_CPU_FLAG_PRESENT      = 0x1,
	MP_CPU_FLAG_BOOTSTRAPPER = 0x2,
};

typedef struct mp_float {
	uint8_t  signature[4];
	uint32_t config_table;
	uint8_t  length;
	uint8_t  revision;
	uint8_t  checksum;
	uint8_t  default_config;
	uint32_t features;
} __attribute__((packed)) mp_float_t;

typedef struct mp_config {
	uint8_t  signature[4];
	uint16_t length;
	uint8_t  revision;
	uint8_t  checksum;
	uint8_t  oem[8];
	uint8_t  product[12];
	uint32_t oem_table;
	uint16_t oem_table_size;
	uint16_t entry_count;
	uint32_t lapic_addr;
	uint16_t extended_length;
	uint8_t  extended_checksum;
	uint8_t  reserved;
} __attribute__((packed)) mp_config_t;

typedef struct mp_cpu_entry {
	uint8_t  type;
	uint8_t  local_apic_id;
	uint8_t  local_apic_version;
	uint8_t  flags;
	uint32_t signature;
	uint32_t features;
	uint64_t reserved;
} __attribute__((packed)) mp_cpu_entry_t;

typedef struct mp_bus {
	uint8_t  type;
	uint8_t  id;
	uint8_t  bus_type[6];
} __attribute__((packed)) mp_bus_t;

typedef struct mp_io_apic {
	uint8_t  type;
	uint8_t  id;
	uint8_t  version;
	uint8_t  flags;
	uint32_t address;
} __attribute__((packed)) mp_io_apic_t;

typedef struct mp_interrupt {
	uint8_t  type;
	uint8_t  int_type;
	uint16_t flags;
	uint8_t  source_bus_id;
	uint8_t  source_bus_irq;
	uint8_t  dest_apic_id;
	uint8_t  dest_apic_intin;
} __attribute__((packed)) mp_interrupt_t;

void mp_boot_cpu( void *lapic, mp_cpu_entry_t *cpu, unsigned cpu_num );
void mp_handle_cpu( void *lapic, void *ptr );
void mp_handle_bus( void *lapic, void *ptr );
void mp_handle_ioapic( void *lapic, void *ptr );
void mp_handle_interrupt( void *lapic, void *ptr );
mp_float_t *mp_find( void );
void mp_enumerate( mp_float_t *mp );

#endif
