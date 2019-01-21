#ifndef _C4_KOBJECT_H
#define _C4_KOBJECT_H 1
#include <stdint.h>

// TODO: Duplicating field and enum from capability.h, should we be storing
//       redundant type info? There does need to be a type field in the
//       kobject_t so the garbage collector knows what to do with things...
enum {
	KOBJECT_TYPE_NULL,
	KOBJECT_TYPE_IPC_SYNC_ENDPOINT,
	KOBJECT_TYPE_IPC_ASYNC_ENDPOINT,
	KOBJECT_TYPE_CAP_SPACE,
	KOBJECT_TYPE_ADDR_SPACE,
	KOBJECT_TYPE_THREAD,
	KOBJECT_TYPE_IO_PORT,
	KOBJECT_TYPE_PHYS_MEMORY,
	KOBJECT_TYPE_RESERVED,
};

typedef struct kobject kobject_t;

#include <c4/syncronization.h>

typedef struct kobject {
    // TODO: back/front links for garbage collection
    mutex_t mutex;
    uint16_t type;
} kobject_t;

// wrapping lock functions here so that fancy debugging things can be done,
// e.g. keeping track of what object a thread is waiting to access, and
// having the possibility of unlocking an object if a thread faults in the
// kernel
static inline void kobject_lock(kobject_t *obj) {
    mutex_lock(&obj->mutex);
}

static inline void kobject_unlock(kobject_t *obj) {
    mutex_unlock(&obj->mutex);
}

#endif
