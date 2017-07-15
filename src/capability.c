#include <c4/mm/region.h>
#include <c4/mm/slab.h>
#include <c4/capability.h>
#include <stddef.h>

static slab_t cap_space_slab;

void cap_space_init( void ){
	static bool initialized = false;

	if ( !initialized ){
		slab_init_at( &cap_space_slab, region_get_global(),
		              sizeof(cap_space_t), NO_CTOR, NO_DTOR );

		initialized = true;
	}
}

cap_space_t *cap_space_create( void ){
	cap_space_t *ret = slab_alloc( &cap_space_slab );

	if ( ret ){
		cap_table_t *table = region_alloc( region_get_global( ));

		if ( !table ){
			slab_free( &cap_space_slab, ret );
			return NULL;
		}

		// Insert the capability space into it's own table as entry 0
		cap_entry_t entry = {
			.type        = CAP_TYPE_CAP_SPACE,
			.permissions = CAP_ACCESS | CAP_MODIFY | CAP_SHARE | CAP_MULTI_USE,
			.object      = ret,
		};

		ret->table = table;
		cap_table_replace( ret->table, 0, &entry );
	}

	return ret;
}

void cap_space_free( cap_space_t *space ){
	// TODO: reference counting
	region_free( region_get_global(), space->table );
	slab_free( &cap_space_slab, space );
}

cap_entry_t *cap_space_lookup( cap_space_t *space, uint32_t object ){
	return cap_table_lookup( space->table, object );
}

uint32_t cap_space_share( cap_space_t *source,
                          cap_space_t *dest,
                          uint32_t object,
                          uint32_t permissions )
{
	cap_entry_t *entry = cap_table_lookup( source->table, object );

	if ( !entry || (entry->permissions & CAP_SHARE) == false ){
		return CAP_INVALID_OBJECT;
	}

	return cap_space_insert( dest, entry );
}

uint32_t cap_space_insert( cap_space_t *space, cap_entry_t *entry ){
	cap_entry_t *space_entry = cap_space_root_entry( space );

	if ( space_entry->permissions & CAP_MODIFY ){
		return cap_table_insert( space->table, entry );
	}

	return CAP_INVALID_OBJECT;
}

bool cap_space_replace( cap_space_t *space,
                        uint32_t object,
                        cap_entry_t *entry )
{
	cap_entry_t *old_entry = cap_table_lookup( space->table, object );

	if ( !old_entry ){
		return false;
	}

	// find the owning capability space of the object to make sure we have
	// permission to modify it's entries
	cap_space_t *root_space = cap_entry_root_space( old_entry );
	cap_entry_t *space_entry = cap_space_root_entry( root_space );

	if ( space_entry->permissions & CAP_MODIFY ){
		*old_entry = *entry;
		return true;
	}

	return false;
}

bool cap_space_remove( cap_space_t *space, uint32_t object ){
	cap_entry_t *entry = cap_table_lookup( space->table, object );

	if ( !entry ){
		return false;
	}

	cap_space_t *root_space = cap_entry_root_space( entry );
	cap_entry_t *space_entry = cap_space_root_entry( root_space );

	if ( space_entry->permissions & CAP_MODIFY ){
		entry->type = CAP_TYPE_NULL;
		return true;
	}

	return false;
}

bool cap_space_restrict( cap_space_t *space,
                         uint32_t object,
                         uint32_t permissions )
{
	cap_entry_t *entry = cap_table_lookup( space->table, object );

	if ( !entry ){
		return false;
	}

	cap_space_t *root_space = cap_entry_root_space( entry );
	cap_entry_t *space_entry = cap_space_root_entry( root_space );

	// check that the capability space can be modified
	if ( (space_entry->permissions & CAP_MODIFY) == false ){
		return false;
	}

	// ensure that the specified permissions are a subset of the current
	// permissions it already has
	if ( (entry->permissions & permissions) != permissions ){
		return false;
	}

	entry->permissions = permissions;
	return true;
}

// XXX: capability tables are page-aligned, so this returns the address of
//      the page `entry` is contained in, which will contain the first
//      entry of the capability table as a result
cap_space_t *cap_entry_root_space( cap_entry_t *entry ){
	uintptr_t ptr = (uintptr_t)entry;

	return (void *)(ptr & ~(PAGE_SIZE - 1));
}

cap_entry_t *cap_space_root_entry( cap_space_t *space ){
	return space->table->entries;
}

// TODO: should access permissions be checked here when recursing into other
//       capability spaces?
cap_entry_t *cap_table_lookup( cap_table_t *table, uint32_t object ){
	cap_entry_t *entry = NULL;
	bool found = false;

	do {
		uint32_t offset;

		offset = object % CAP_ENTRIES_PER_PAGE;
		object = object / CAP_ENTRIES_PER_PAGE;
		entry = table->entries + offset;

		if ( object == 0 && offset == 0 ){
			// return the first entry, which is the self-referring
			// capibility space entry
			found = true;

		// recurse into another capability space and continue searching
		} else if ( entry->type == CAP_TYPE_CAP_SPACE ){
			cap_space_t *space = entry->object;
			table = space->table;

		// the object is not a capability space, so having a sub-object in
		// the address makes no sense
		} else if ( object != 0 ){
			entry = NULL;

		// otherwise, we found whatever it was we're looking for
		} else {
			found = true;
		}

	} while ( entry && !found );

	return entry;
}

// TODO: optimization: keep track of the first capability slot which is unused,
//       or even a list of all unused blocks.
//
//       a list actually sounds like a really good idea, could keep a linked
//       list which would make insertion and removal O(1), with very little
//       overhead even cache-wise since all the data will reside on the same
//       page.
//
// XXX: the first entry in the capability table will always refer to the
//      holding capability space, so 0 will be returned as an error code by
//      functions which return an object address

uint32_t cap_table_insert( cap_table_t *table, cap_entry_t *entry ){
	for ( unsigned i = 0; i < CAP_ENTRIES_PER_PAGE; i++ ){
		if ( table->entries[i].type != CAP_TYPE_NULL ){
			continue;
		}

		*(table->entries + i) = *entry;
		return i;
	}

	return CAP_INVALID_OBJECT;
}

void cap_table_remove( cap_table_t *table, uint32_t object ){
	uint32_t offset = object % CAP_ENTRIES_PER_PAGE;
	cap_entry_t *entry = table->entries + offset;

	entry->type = CAP_TYPE_NULL;
}

void cap_table_replace( cap_table_t *table,
                        uint32_t object,
                        cap_entry_t *entry )
{
	uint32_t offset = object % CAP_ENTRIES_PER_PAGE;
	cap_entry_t *old_entry = table->entries + offset;

	*old_entry = *entry;
}
