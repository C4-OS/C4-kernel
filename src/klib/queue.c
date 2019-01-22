#include <c4/klib/queue.h>
#include <c4/mm/slab.h>
#include <c4/klib/string.h>
#include <stdlib.h>
#include <stdbool.h>

static slab_t queue_slab;
static bool slab_initialized = false;

static inline void slab_init(void) {
	if (!slab_initialized) {
		slab_initialized = true;
		slab_init_at(&queue_slab,
		             region_get_global(),
		             sizeof(queue_node_t),
		             NO_CTOR, NO_DTOR);
	}
}

queue_t *queue_init(queue_t *queue) {
	memset(queue, 0, sizeof(queue_t));
	return queue;
}

queue_t *queue_create(void) {
	slab_init();
	return slab_alloc(&queue_slab);
}

queue_node_t *queue_node_create(void *data) {
	slab_init();

	queue_node_t *ret = slab_alloc(&queue_slab);
	ret->data = data;
	ret->next = ret->prev = NULL;
	return ret;
}

void queue_node_free(queue_node_t *node) {
	slab_free(&queue_slab, node);
}

void queue_push_front(queue_t *queue, void *data){
	queue_node_t *node = queue_node_create(data);

	node->next = queue->front;
	queue->front = node;
	queue->items++;

	if (!queue->back) {
		queue->back = node;

	} else {
		node->next->prev = node;
	}
}

void queue_push_back(queue_t *queue, void *data){
	queue_node_t *node = queue_node_create(data);

	node->prev = queue->back;
	queue->back = node;
	queue->items++;

	if (!queue->front) {
		queue->front = node;

	} else {
		node->prev->next = node;
	}
}

void *queue_pop_front(queue_t *queue) {
	queue_node_t *node = queue->front;

	if (!node) {
		return NULL;
	}

	queue->front = node->next;
	queue->items--;

	if (!queue->front) {
		// no more items in the queue
		//assert(queue->items == 0);
		queue->back = NULL;

	} else {
		node->next->prev = NULL;
	}

	void *ret = node->data;
	queue_node_free(node);
	return ret;
}

void *queue_pop_back(queue_t *queue) {
	queue_node_t *node = queue->back;

	if (!node) {
		return NULL;
	}

	queue->back = node->prev;
	queue->items--;

	if (!queue->back) {
		// no more items in the queue
		//assert(queue->items == 0);
		queue->front = NULL;

	} else {
		node->prev->next = NULL;
	}

	void *ret = node->data;
	queue_node_free(node);
	return ret;
}

void *queue_peek_front(queue_t *queue) {
	return queue->front;
}

void *queue_peek_back(queue_t *queue) {
	return queue->back;
}

typedef int (*queue_compare)(void *a, void *b);

void *queue_pop_min(queue_t *a, queue_t *b, queue_compare comp) {
	if (a->items == 0 && b->items == 0) {
		return NULL;
	}

	if (a->items == 0) return queue_pop_front(b);
	if (b->items == 0) return queue_pop_front(a);

	int diff = comp(queue_peek_front(a), queue_peek_front(b));

	return queue_pop_front((diff <= 0)? a : b);
}

