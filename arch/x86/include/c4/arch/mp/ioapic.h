#ifndef _MP_IOAPIC_H
#define _MP_IOAPIC_H 1

enum {
	IOAPIC_REG_ID           = 0x0,
	IOAPIC_REG_VERSION      = 0x1,
	IOAPIC_REG_PRIORITY     = 0x2,
	IOAPIC_REG_REDIRECT     = 0x10,
	IOAPIC_REG_REDIRECT_END = 0x3f,
};

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

#endif
