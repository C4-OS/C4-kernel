#ifndef _C4_COMMON_H
#define _C4_COMMON_H 1

#define NULL ((void *)0)

// some generic macros for setting flags in structs with a 'flags' field
#define SET_FLAG(THING, FIELD) \
	((THING)->flags |= (FIELD))

#define UNSET_FLAG(THING, FIELD) \
	((THING)->flags &= ~(FIELD))

#define FLAG(THING, FIELD) \
	(!!((THING)->flags & (FIELD)))

#endif
