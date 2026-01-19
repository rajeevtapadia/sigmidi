#ifndef RINGBUF_H
#define RINGBUF_H

#include <assert.h>
#include <stdbool.h>

#define RINGBUF_VERSION 1.1.0

#define INIT_CAP 10

struct RingBuf {
    void *items;
    int out; // index of next writable place
    int in;  // index of start of buffer
    int size;
    int capacity;
    int item_size;
};

/*
 *       Ring Buffer
 *  Read <---------< Write
 *       ^         ^
 *       |         |
 *      out        in
 */

struct RingBuf ringbuf_alloc(int item_size);
void ringbuf_push(struct RingBuf *rb, void *item);
bool ringbuf_is_full(struct RingBuf *rb);
bool ringbuf_is_empty(struct RingBuf *rb);
void ringbuf_resize(struct RingBuf *rb, int new_size);
void ringbuf_pop(struct RingBuf *rb, void *popped_event);
void ringubf_peek(struct RingBuf *rb, void *item);
void ringbuf_free(struct RingBuf *rb);
// void ringbuf_dump(struct RingBuf *rb);

#endif // RINGBUF_H

#ifdef RINGBUF_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#define RINGBUF_AT(rb, idx) ((unsigned char *)(rb)->items + (rb)->item_size * (idx))

struct RingBuf ringbuf_alloc(int item_size) {
    assert(item_size > 0);

    struct RingBuf rb;
    rb.items = (void *)malloc(item_size * INIT_CAP);
    rb.out = 0;
    rb.in = 0;
    rb.size = 0;
    rb.capacity = INIT_CAP;
    rb.item_size = item_size;

    return rb;
}

void ringbuf_push(struct RingBuf *rb, void *item) {
    assert(rb != NULL);
    assert(item != NULL);

    if (ringbuf_is_full(rb)) {
        ringbuf_resize(rb, rb->capacity * 1.5);
    }

    memcpy(RINGBUF_AT(rb, rb->in), item, rb->item_size);

    rb->in = (rb->in + 1) % (rb->capacity);
    rb->size++;
}

inline bool ringbuf_is_full(struct RingBuf *rb) {
    return rb->size == rb->capacity;
}

inline bool ringbuf_is_empty(struct RingBuf *rb) {
    return rb->size == 0;
}

void ringbuf_resize(struct RingBuf *rb, int new_size) {
    assert(new_size > rb->capacity);

    void *new_buf = malloc(sizeof(rb->item_size) * new_size);

    for (int i = 0; i < rb->size; i++) {
        int rb_idx = (rb->out + i) % rb->capacity;
        void *new_buf_itr = (unsigned char *)new_buf + (i * rb->item_size);
        memcpy(new_buf_itr, RINGBUF_AT(rb, rb_idx), rb->item_size);
    }

    free(rb->items);
    rb->items = new_buf;
    rb->out = 0;
    rb->in = rb->size;
    rb->capacity = new_size;
}

void ringbuf_pop(struct RingBuf *rb, void *popped_event) {
    assert(rb != NULL);
    assert(!ringbuf_is_empty(rb));

    if (popped_event != NULL) {
        memcpy(popped_event, RINGBUF_AT(rb, rb->out), rb->item_size);
    }

    rb->out = (rb->out + 1) % rb->capacity;
    rb->size--;
}

void ringubf_peek(struct RingBuf *rb, void *item) {
    assert(!ringbuf_is_empty(rb));

    memcpy(item, RINGBUF_AT(rb, rb->out), rb->item_size);
}

void ringbuf_free(struct RingBuf *rb) {
    free(rb->items);
    rb->items = NULL;
    rb->capacity = rb->size = rb->out = rb->in = 0;
}

// void ringbuf_dump(struct RingBuf *rb) {
//   fprintf(stderr, "--------------------RingBuf Dump--------------------");
//
//   for (int i = 0; i < rb->capacity; i++) {
//     if (i % 4 == 0)
//       fprintf(stderr, "\n");
//
//     int rb_idx = (rb->out + i) % rb->capacity;
//     fprintf(stderr, "%p ", rb->items[rb_idx]);
//   }
//   fprintf(stderr, "\n-----------------------------------------------------\n");
// }
#endif // RINGBUF_IMPLEMENTATION
