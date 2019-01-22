#ifndef _C4_QUEUE_H
#define _C4_QUEUE_H 1

#include <stddef.h>
#include <stdint.h>

typedef struct queue_node queue_node_t;
typedef struct queue_node {
	union {
		void *data;
		uintptr_t value;
	};

	queue_node_t *next;	
	queue_node_t *prev;	
} queue_node_t;

typedef struct queue {
	queue_node_t *front;
	queue_node_t *back;

	size_t items;
} queue_t;

queue_t *queue_init(queue_t *queue);
queue_t *queue_create(void);
queue_node_t *queue_node_create(void *data);
void queue_push_front(queue_t *queue, void *data);
void queue_push_back(queue_t *queue, void *data);
void *queue_pop_front(queue_t *queue);
void *queue_pop_back(queue_t *queue);
void *queue_peek_front(queue_t *queue);
void *queue_peek_back(queue_t *queue);
typedef int (*queue_compare)(void *a, void *b);
void *queue_pop_min(queue_t *a, queue_t *b, queue_compare comp);

// XXX: we don't need no stinking c++ templates...
#define TYPESAFE_QUEUE(TYPE, OUTTYPE) \
	typedef struct { queue_t queue; } OUTTYPE; \
\
	static inline OUTTYPE* OUTTYPE##_init(OUTTYPE *queue) { \
		queue_init(&queue->queue); \
	} \
\
	static inline void OUTTYPE##_push_front(OUTTYPE *queue, TYPE *data) { \
		queue_push_front(&queue->queue, data); \
	} \
\
	static inline void OUTTYPE##_push_back(OUTTYPE *queue, TYPE *data) { \
		queue_push_back(&queue->queue, data); \
	} \
\
	static inline TYPE* OUTTYPE##_pop_front(OUTTYPE *queue) { \
		return queue_pop_front(&queue->queue); \
	} \
\
	static inline TYPE* OUTTYPE##_pop_back(OUTTYPE *queue) { \
		return queue_pop_back(&queue->queue); \
	} \
\
	static inline TYPE* OUTTYPE##_peek_front(OUTTYPE *queue) { \
		return queue_peek_front(&queue->queue); \
	} \
\
	static inline TYPE* OUTTYPE##_peek_back(OUTTYPE *queue) { \
		return queue_peek_back(&queue->queue); \
	}

#endif
