#ifndef _C4_CAPABILITY_H
#define _C4_CAPABILITY_H 1
#include <c4/mm/region.h>
#include <c4/paging.h>
#include <stdint.h>

// Capability permissions, for use in the `permissions` bitfield
enum {
    CAP_ACCESS    = 1 << 0,
    CAP_MODIFY    = 1 << 1,
    CAP_SHARE     = 1 << 2,
    CAP_MULTI_USE = 1 << 3,
};

// Capability types, used in the `type` bitfield
enum {
    CAP_TYPE_NULL,
    CAP_TYPE_IPC_SYNC_ENDPOINT,
    CAP_TYPE_IPC_ASYNC_ENDPOINT,
    CAP_TYPE_CAP_SPACE,
    CAP_TYPE_ADDR_SPACE,
    CAP_TYPE_THREAD,
    CAP_TYPE_IO_PORT,
    CAP_TYPE_PHYS_MEMORY,
};

enum {
	CAP_INVALID_OBJECT = 0,
};

typedef struct cap_entry {
    // make sure the entry uses 16 bytes of memory
    uint32_t padding[4];

    union {
        struct {
            uint32_t type;
            uint32_t permissions;
            void *object;
        };
    };

} __attribute__((packed)) cap_entry_t;

// Determine how many entries will fit in a page, since the cap_table_t
// structure will be stored in one full page
enum {
    CAP_ENTRIES_PER_PAGE = PAGE_SIZE / sizeof(cap_entry_t),
};

// XXX: the first entry in the capability table will always refer to the
//      holding capability space, so 0 will be returned as an error code by
//      functions which return an object address

typedef struct cap_table {
    cap_entry_t entries[CAP_ENTRIES_PER_PAGE];
} __attribute__((packed)) cap_table_t;

typedef struct cap_space {
    cap_table_t *table;

    unsigned references;
} cap_space_t;

cap_entry_t *cap_table_lookup( cap_table_t *table, uint32_t object );
uint32_t     cap_table_insert( cap_table_t *table, cap_entry_t *entry );
void         cap_table_remove( cap_table_t *table, uint32_t object );
void         cap_table_replace( cap_table_t *table,
                                uint32_t object,
                                cap_entry_t *entry );

cap_space_t *cap_entry_root_space( cap_entry_t *entry );

void         cap_space_init( void );
cap_space_t *cap_space_create( void );
void         cap_space_free( cap_space_t *space );
uint32_t     cap_space_share( cap_space_t *source,
                              cap_space_t *dest,
                              uint32_t object,
                              uint32_t permissions );
uint32_t     cap_space_insert( cap_space_t *space, cap_entry_t *entry );
bool         cap_space_replace( cap_space_t *space,
                                uint32_t object,
                                cap_entry_t *entry );
bool         cap_space_remove( cap_space_t *space, uint32_t object );
bool         cap_space_restrict( cap_space_t *space,
                                 uint32_t object,
                                 uint32_t permissions );
cap_entry_t *cap_space_root_entry( cap_space_t *space );

#endif
