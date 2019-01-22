#ifndef _C4_ANDTREE_H
#define _C4_ANDTREE_H 1
#include <stdbool.h>

typedef struct c4_anode c4_anode_t;
typedef struct c4_atree c4_atree_t;

typedef int (*c4_atree_get_key_t)(void *b);

struct c4_anode {
	c4_anode_t *parent;
	c4_anode_t *leaves[2];
	unsigned level;
	void *data;
};

struct c4_atree {
	c4_anode_t *root;
	unsigned nodes;
	c4_atree_get_key_t get_key;
};

c4_anode_t *atree_insert( c4_atree_t *tree, void *data );

void atree_remove( c4_atree_t *tree, c4_anode_t *node );
void *atree_remove_key( c4_atree_t *tree, int key );
void *atree_remove_data( c4_atree_t *tree, void *data );

c4_atree_t *atree_init( c4_atree_t *tree,
                        c4_atree_get_key_t compare );
c4_atree_t *atree_deinit( c4_atree_t *tree );
void atree_print( c4_atree_t *tree );
bool atree_check( c4_atree_t *tree );

c4_anode_t *atree_find_at_least( c4_atree_t *tree, int key );
c4_anode_t *atree_find_data( c4_atree_t *tree, void *data );
c4_anode_t *atree_find_key( c4_atree_t *tree, int key );

c4_anode_t *atree_start(c4_atree_t *tree);
c4_anode_t *atree_end(c4_atree_t *tree);
c4_anode_t *atree_next(c4_anode_t *node);
c4_anode_t *atree_previous(c4_anode_t *node);

#endif
