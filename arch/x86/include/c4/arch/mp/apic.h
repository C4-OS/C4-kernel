#ifndef _MP_APIC_H
#define _MP_APIC_H 1
#include <stdint.h>
#include <stdbool.h>

enum {
	APIC_REG_ID                 = 0x20,
	APIC_REG_VERSION            = 0x30,
	APIC_REG_TASK_PRIORITY      = 0x80,
	APIC_REG_ARB_PRIORITY       = 0x90,
	APIC_REG_PROC_PRIORITY      = 0xa0,
	APIC_REG_END_OF_INTR        = 0xb0,
	APIC_REG_REMOTE_READ        = 0xc0,
	APIC_REG_LOGICAL_DEST       = 0xd0,
	APIC_REG_DEST_FORMAT        = 0xe0,
	APIC_REG_SPURIOUS_INTR_VEC  = 0xf0,
	APIC_REG_IN_SERVICE         = 0x100, /* range from 0x100 to 0x170 */
	APIC_REG_TRIGGER_MODE       = 0x180, /* range from 0x180 to 0x1f0 */
	APIC_REG_INTR_REQUEST       = 0x200, /* range from 0x200 to 0x270 */
	APIC_REG_ERROR_STATUS       = 0x280,
	APIC_REG_INTR_COMMAND_UPPER = 0x300,
	APIC_REG_INTR_COMMAND_LOWER = 0x310,
	APIC_REG_LOCAL_TIMER        = 0x320,
	APIC_REG_LOCAL_THERMAL      = 0x330,
	APIC_REG_LOCAL_PERF         = 0x340,
	APIC_REG_LOCAL_INTR0        = 0x350,
	APIC_REG_LOCAL_INTR1        = 0x360,
	APIC_REG_ERROR_VEC          = 0x370,
	APIC_REG_TIMER_INIT_COUNT   = 0x380,
	APIC_REG_TIMER_CUR_COUNT    = 0x390,
	APIC_REG_TIMER_DIV_CONF     = 0x3e0,
	APIC_REG_EXT_FEATURE        = 0x400,
	APIC_REG_EXT_CONTROL        = 0x410,
	APIC_REG_SPEC_END_OF_INTR   = 0x420,
	APIC_REG_INTR_ENABLE        = 0x480, /* range from 0x480 to 0x4f0 */
	APIC_REG_EXT_INTR_LOCAL_VEC = 0x500, /* range from 0x500 to 0x530 */
};

enum {
	APIC_ICR_DM              = 1 << 11,
	APIC_ICR_LEVEL_ASSERT    = 1 << 14,
	APIC_ICR_LEVEL           = 1 << 15,
	APIC_ICR_DELIVERY_STATUS = 1 << 12,
};

enum {
	APIC_IPI_INIT  = 0x500,
	APIC_IPI_START = 0x600,
};

uint32_t apic_read( void *addr, uint32_t reg );
void apic_write( void *addr, uint32_t reg, uint32_t value );
void apic_send_ipi( void *addr, uint32_t type, uint8_t lapic_id );
bool apic_ipi_recieved( void *apic );
bool apic_supported( void );
void apic_enable( void );
uint32_t apic_version( void );
uint32_t apic_get_id( void );
void apic_timer( void );

#endif
