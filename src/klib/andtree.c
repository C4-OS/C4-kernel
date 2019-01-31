#include <c4/klib/andtree.h>
#include <c4/klib/string.h>
#include <c4/mm/slab.h>
#include <c4/debug.h>
#include <stdbool.h>

static c4_anode_t sentinel = {
	.level = 0,
	.leaves = { &sentinel, &sentinel, },
	.parent = NULL,
	.data = NULL,
};

static slab_t atree_slab;
static bool slab_initialized = false;

// values for indexing into leaf array in nodes, for readability
enum { LEFT, RIGHT };

static inline bool is_nil(c4_anode_t *node){
	return node == NULL || node->level == 0;
}

static inline unsigned parentside( c4_anode_t *node ){
	if (!node || !node->parent) {
		return RIGHT;
	}

	return node->parent->leaves[0] != node;
}


static inline unsigned direction( int64_t n ){
	return !(n <= 0);
}

static inline
int64_t atree_compare( c4_atree_t *tree, int64_t key, c4_anode_t *node ){
	return key - tree->get_key(node->data);
}

static inline c4_anode_t *find_min_node( c4_anode_t *node ){
	if (node->level == 0) {
		return NULL;
	}

    for ( ; !is_nil(node->leaves[LEFT]); node = node->leaves[LEFT] );

    return node;
}

static inline c4_anode_t *find_max_node( c4_anode_t *node ){
	if (is_nil(node)) {
		return NULL;
	}

    for ( ; !is_nil(node->leaves[RIGHT]); node = node->leaves[RIGHT] );

    return node;
}

c4_anode_t *atree_find_key( c4_atree_t *tree, int64_t key ){
    c4_anode_t *temp = tree->root;

    while (!is_nil(temp)) {
		int64_t diff = atree_compare(tree, key, temp);

        if (diff == 0) {
            return temp;
        }

		temp = temp->leaves[direction(diff)];
    }

    return NULL;
}

// fallback recursive search for trees with duplicate elements
static inline
c4_anode_t *atree_find_dupe_data( c4_atree_t *tree,
                                       c4_anode_t *node,
                                       void *data )
{
	if (is_nil(node)) {
		return NULL;
	}

	if (node->data == data) {
		return node;
	}

	int64_t diff = atree_compare(tree, tree->get_key(data), node);

	if (diff == 0) {
		for (unsigned i = 0; i < 2; i++) {
			c4_anode_t *a = atree_find_dupe_data(tree,
			                                          node->leaves[i],
			                                          data);

			if (a) {
				return a;
			}
		}
	}

	return atree_find_dupe_data(tree, node->leaves[direction(diff)], data);
}

c4_anode_t *atree_find_data( c4_atree_t *tree, void *data ){
	c4_anode_t *temp = tree->root;

	while (!is_nil(temp)) {
		int64_t diff = atree_compare(tree, tree->get_key(data), temp);

		if (diff == 0){
			return atree_find_dupe_data(tree, temp, data);
		}

		temp = temp->leaves[direction(diff)];
	}

	return NULL;
}

static c4_anode_t *do_find_at_least( c4_atree_t *tree,
                                       c4_anode_t *node, 
                                       int64_t key )
{
	if (is_nil(node)) {
		return NULL;
	}

	int64_t diff = atree_compare(tree, key, node);
	unsigned dir = direction(diff);

	// found the exact value we're looking for, return it
	if (diff == 0) {
		return node;
	}

	c4_anode_t *temp = do_find_at_least(tree, node->leaves[dir], key);

	if (diff < 0) {
		c4_anode_t *ret = (temp && atree_compare(tree, key, temp) > diff)? temp : node;
		return ret;
	}

	return temp;
}

c4_anode_t *atree_find_at_least( c4_atree_t *tree, int64_t key ){
	return do_find_at_least(tree, tree->root, key);
}

c4_anode_t *atree_start(c4_atree_t *tree){
	return find_min_node(tree->root);
}

c4_anode_t *atree_end(c4_atree_t *tree){
	return find_max_node(tree->root);
}

c4_anode_t *atree_next(c4_anode_t *node){
	if (!node){
		return node;
	}

	c4_anode_t *right = find_min_node(node->leaves[RIGHT]);
	c4_anode_t *last = node;
	c4_anode_t *p = node->parent;

	// node has right nodes to check out
	if (!is_nil(right)) {
		return right;
	}

	while (!is_nil(p) && p->leaves[RIGHT] == last){
		last = p, p = p->parent;
	}

	return p;
}

c4_anode_t *atree_previous(c4_anode_t *node){
	if (!node){
		return node;
	}

	c4_anode_t *left = find_max_node(node->leaves[LEFT]);
	c4_anode_t *last = node;
	c4_anode_t *p = node->parent;

	if (!is_nil(left)) {
		return left;
	}

	while (!is_nil(p) && p->leaves[LEFT] == last){
		last = p, p = p->parent;
	}

	return p;
}

static inline void replace_in_parent( c4_anode_t *node,
                                      c4_anode_t *value )
{
	if (node->parent) {
		node->parent->leaves[parentside(node)] = value;
	}

	if (!is_nil(value)) {
		value->parent = node->parent;
	}
}

static inline
c4_anode_t *rotate( c4_atree_t *tree, c4_anode_t *node, unsigned dir ){
	c4_anode_t *temp = node->leaves[!dir];

	node->leaves[!dir] = temp->leaves[dir];
	temp->leaves[dir] = node;

	if (node->leaves[!dir]){
		node->leaves[!dir]->parent = node;
	}

	replace_in_parent(node, temp);
	node->parent = temp;

    if (node == tree->root){
        tree->root = temp;
    }

	return temp;
}

static void do_node_print( c4_atree_t *tree,
                           c4_anode_t *node,
                           unsigned indent );


static inline
c4_anode_t *atree_skew(c4_atree_t *tree, c4_anode_t *node){
	c4_anode_t *ret = node;

	if (is_nil(node)) {
		return ret;
	}

	if (node->leaves[LEFT]->level == node->level) {
		ret = rotate(tree, node, RIGHT);
		atree_skew(tree, ret->leaves[RIGHT]);
	}

	return ret;
}

static inline
c4_anode_t *atree_split(c4_atree_t *tree, c4_anode_t *node){
	c4_anode_t *ret = node;

	if (is_nil(node)) {
		return node;
	}

	if (node->level == node->leaves[RIGHT]->leaves[RIGHT]->level) {
		ret = rotate(tree, node, LEFT);
		ret->level += 1;
		atree_split(tree, ret->leaves[RIGHT]);
	}

	return ret;
}

static inline void do_remove_node( c4_atree_t *tree,
                                   c4_anode_t *node,
                                   c4_anode_t *replacement )
{
	// TODO: assert()
	//assert(tree->nodes > 0);

	if (node == tree->root) {
		tree->root = replacement;
	}

	replace_in_parent(node, replacement);
	slab_free(&atree_slab, node);
	tree->nodes--;

}

static inline unsigned count_nodes( c4_anode_t *node ){
	if (node && node->level != 0) {
		return count_nodes(node->leaves[LEFT])
		     + count_nodes(node->leaves[RIGHT])
		     + 1;
	}

	else return 0;
}

void atree_bin_insert( c4_atree_t *tree, c4_anode_t *node ){
    //assert(!is_nil(node));

	if (is_nil(tree->root)) {
		tree->root = node;
		return;
	}

	c4_anode_t *temp = tree->root;

	while (temp != node) {
		int64_t diff = atree_compare(tree, tree->get_key(node->data), temp);
		unsigned dir = !(diff < 0);

		if (is_nil(temp->leaves[dir])) {
			temp->leaves[dir] = node;
			node->parent = temp;
		}

		temp = temp->leaves[dir];
	}
}

static inline
void atree_insert_repair( c4_atree_t *tree, c4_anode_t *node ){
	c4_anode_t *temp = node;

	while (temp) {
		temp = atree_skew(tree, temp);
		temp = atree_split(tree, temp);
		temp = temp->parent;
	}
}

static inline
void atree_remove_repair( c4_atree_t *tree, c4_anode_t *node ){
	c4_anode_t *successor = &sentinel;
	c4_anode_t *repair = NULL;

	//assert(node != NULL);

	successor = find_max_node(node->leaves[LEFT]);
	successor = successor? successor : find_min_node(node->leaves[RIGHT]);

	{
		c4_anode_t *temp = (!is_nil(successor))? successor : node;

		repair = temp->parent;

		if (successor) {
			node->data = successor->data;
			do_remove_node(tree, successor,
			               successor->leaves[is_nil(successor->leaves[LEFT])]);
		}

		else {
			do_remove_node(tree, node, &sentinel);
		}
	}

	for (; !is_nil(repair); repair = repair->parent) {
		if ((repair->leaves[LEFT]->level < repair->level - 1)
		 || (repair->leaves[RIGHT]->level < repair->level - 1))
		{
			repair->level -= 1;

			if (repair->leaves[RIGHT]->level > repair->level){
				repair->leaves[RIGHT]->level = repair->level;
			}

			repair = atree_skew(tree, repair);
			repair = atree_split(tree, repair);

			// XXX: rarely a split will end up with too many right
			//      horizonal links in the left sub-tree, which when fixing
			//      may result in a left horizonal link which needs to be
			//      skewed...
			//
			//      can't quite figure out what I'm doing that's different
			//      from other implementations that makes this necessary, but
			//      it works, it's 5AM, my brain not work so good and
			//      it's still O(log n) so I'll leave it for now
			atree_split(tree, repair->leaves[LEFT]);
			repair = atree_skew(tree, repair);
		}
	}
}


static inline c4_anode_t *make_anode( void *data ){
	c4_anode_t *ret = slab_alloc(&atree_slab);

	memset(ret, 0, sizeof(*ret));
	ret->data = data;
	ret->level = 1;
	ret->leaves[LEFT] = ret->leaves[RIGHT] = &sentinel;

	return ret;
}

bool atree_check( c4_atree_t *tree );

c4_anode_t *atree_insert( c4_atree_t *tree, void *data ){
	c4_anode_t *node = make_anode( data );
	//assert(node != NULL);

	atree_bin_insert(tree, node);
	atree_insert_repair(tree, node);
	tree->nodes += 1;

// flag to enable debugging checks, should normally be disabled
#ifdef ATREE_CHECK_OPS
	debug_printf("==> should have %u nodes, have %u\n",
		            tree->nodes, count_nodes(tree->root));

	KASSERT(tree->nodes == count_nodes(tree->root));
	KASSERT(atree_check(tree) == true);
#endif

	return node;
}

void atree_remove( c4_atree_t *tree, c4_anode_t *node ){
	atree_remove_repair(tree, node);

// flag to enable debugging checks, should normally be disabled
#ifdef ATREE_CHECK_OPS
	debug_printf("==> should have %u nodes, have %u\n",
		            tree->nodes, count_nodes(tree->root));
	KASSERT(tree->nodes == count_nodes(tree->root));

	if (!atree_check(tree)){
		debug_puts("----");
		atree_print(tree);
		debug_puts("----");
	}

	KASSERT(atree_check(tree) == true);
#endif
}

void *atree_remove_key( c4_atree_t *tree, int64_t key ){
	c4_anode_t *node = atree_find_key(tree, key);
	void *ret = NULL;

	if (node) {
		ret = node->data;
		atree_remove(tree, node);
	}

	return ret;
}

void *atree_remove_data( c4_atree_t *tree, void *data ){
	c4_anode_t *node = atree_find_data(tree, data);
	void *ret = NULL;

	if (node) {
		ret = node->data;
		atree_remove(tree, node);
	}

	return ret;
}

c4_atree_t *atree_init( c4_atree_t *tree,
                            c4_atree_get_key_t get_key )
{
	if (!slab_initialized) {
		slab_initialized = true;
		slab_init_at(&atree_slab,
		             region_get_global(),
		             sizeof(c4_anode_t),
		             NO_CTOR, NO_DTOR);
	}

    memset(tree, 0, sizeof(*tree));
    tree->get_key = get_key;
	tree->root = &sentinel;

	return tree;
}

c4_atree_t *atree_deinit( c4_atree_t *tree ){
    // TODO: fill this in
	return tree;
}

static void do_node_print( c4_atree_t *tree,
                           c4_anode_t *node,
                           unsigned indent )
{

	if (node->level != 0) {
		do_node_print(tree, node->leaves[0], indent + 1);

		for ( unsigned i = 0; i < indent; i++ ){
			debug_printf("  |");
		}

		debug_printf("- (%u) %p : %p : %d", node->level, node, node->data,
		                                       tree->get_key(node->data));

		debug_printf("\n");

		do_node_print(tree, node->leaves[1], indent + 1);
	}
}

void atree_print( c4_atree_t *tree ){
	do_node_print(tree, tree->root, 0);
}

static bool check_tree( c4_atree_t *tree, c4_anode_t *node ){
	if (is_nil(node)) {
		return true;
	}

	bool left = check_tree(tree, node->leaves[LEFT]);
	bool right = check_tree(tree, node->leaves[RIGHT]);
	bool valid = left && right;

	if (node->level > 1) {
		if (is_nil(node->leaves[LEFT]) && is_nil(node->leaves[RIGHT])) {
			debug_printf(">>> external node with level > 1! (%p)\n", node);
			valid = false;
		}

		for (unsigned i = 0; i < 2; i++) {
			if (is_nil(node->leaves[i])) {
				continue;
			}

			if (node->leaves[i]->level < node->level - 1) {
				debug_printf(">>> missing pseudonode! (%p, direction: %u)\n", node, i);
				valid = false;
			}

			if (node->level < node->leaves[i]->level) {
				debug_printf(">>> invalid levels! (%p, direction: %u)\n", node, i);
				valid = false;
			}
		}
	}

	if (node->leaves[LEFT]->level == node->level) {
		debug_printf(">>> horizontal left link! (%p, direction)\n", node);
		valid = false;
	}

	if (node->leaves[RIGHT]->leaves[RIGHT]->level == node->level) {
		debug_printf(">>> too many horizontal right links! (%p, direction)\n", node);
		valid = false;
	}

	return valid;
}

bool atree_check( c4_atree_t *tree ){
	return check_tree(tree, tree->root);
}
