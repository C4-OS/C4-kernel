#ifndef _MP_IOAPIC_H
#define _MP_IOAPIC_H 1
#include <stdint.h>
#include <stdbool.h>

// TODO: maybe have a config.h
#define MAX_IOAPICS 32

enum {
	IOAPIC_REG_ID           = 0x0,
	IOAPIC_REG_VERSION      = 0x1,
	IOAPIC_REG_PRIORITY     = 0x2,
	IOAPIC_REG_REDIRECT     = 0x10,
	IOAPIC_REG_REDIRECT_END = 0x3f,
};

enum {
	IOAPIC_DELIVERY_FIXED           = 0,
	IOAPIC_DELIVERY_LOWEST_PRIORITY = 1,
	IOAPIC_DELIVERY_SMI             = 2,
	IOAPIC_DELIVERY_NMI             = 4,
	IOAPIC_DELIVERY_INIT            = 5,
	IOAPIC_DELIVERY_EXTINT          = 7,
};

enum {
	IOAPIC_DESTINATION_PHYSICAL = 0,
	IOAPIC_DESTINATION_LOGICAL  = 1,
};

typedef unsigned ioapic_t;

typedef struct ioapic_redirect {
	union {
		struct {
			uint32_t lower;
			uint32_t upper;
		};

		struct {
			uint64_t vector           : 8;
			uint64_t delivery         : 3;
			uint64_t destination_mode : 1;
			uint64_t status           : 1;
			uint64_t polarity         : 1;
			uint64_t remote_irr       : 1;
			uint64_t trigger          : 1;
			uint64_t mask             : 1;
			uint64_t reserved         : 39;
			uint64_t destination      : 8;
		};
	};
} __attribute__((packed)) ioapic_redirect_t;

static inline uint32_t ioapic_read( void *addr, uint32_t reg ){
	uint32_t volatile *ioapic = addr;
	ioapic[0] = (reg & 0xff);
	return ioapic[4];
}

static inline void ioapic_write( void *addr, uint32_t reg, uint32_t value ){
	uint32_t volatile *ioapic = addr;
	ioapic[0] = (reg & 0xff);
	ioapic[4] = value;
}

void ioapic_add(void *ioapic, unsigned id);
void *ioapic_get(ioapic_t id);
uint32_t ioapic_version_raw(ioapic_t id);
uint32_t ioapic_version(ioapic_t id);
uint32_t ioapic_max_redirects(ioapic_t id);
ioapic_redirect_t ioapic_get_redirect(ioapic_t id, unsigned entry);
void ioapic_set_redirect(ioapic_t id,
                         unsigned entry,
                         ioapic_redirect_t *redirect);
void ioapic_set_mask(ioapic_t id, unsigned entry, bool mask);
void ioapic_unmask(ioapic_t id, unsigned entry);
void ioapic_mask(ioapic_t id, unsigned entry);

void ioapic_print_redirect(ioapic_t id, unsigned entry);
void ioapic_print_redirects(ioapic_t id);

#endif
