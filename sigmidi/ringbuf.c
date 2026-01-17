#include <alsa/asoundlib.h>
#include <assert.h>
#include <sigmidi.h>
#include <stdlib.h>

#define INIT_CAP 10

struct RingBuf ringbuf_alloc() {
    struct RingBuf rb;
    rb.evts = (struct MidiEvent *)malloc(sizeof(struct MidiEvent) * INIT_CAP);
    rb.start = 0;
    rb.end = 0;
    rb.size = 0;
    rb.capacity = INIT_CAP;

    return rb;
}

void ringbuf_push(struct RingBuf *rb, struct MidiEvent evt) {
    assert(rb != NULL);

    if (ringbuf_is_full(rb)) {
        ringbuf_resize(rb, rb->capacity * 1.5);
    }

    rb->evts[rb->end] = evt;
    rb->end = (rb->end + 1) % (rb->capacity + 1);
    rb->size++;
    ringbuf_print(rb);
}

bool ringbuf_is_full(struct RingBuf *rb) {
    return rb->size == rb->capacity;
}

bool ringbuf_is_empty(struct RingBuf *rb) {
    return rb->size == 0;
}

void ringbuf_resize(struct RingBuf *rb, int new_size) {
    assert(new_size > rb->capacity);

    struct MidiEvent *new_buf = malloc(sizeof(struct MidiEvent) * new_size);

    for (int i = 0; i < rb->size; i++) {
        int ring_buf_idx = (rb->start + i) % rb->capacity;
        new_buf[i] = rb->evts[ring_buf_idx];
    }

    free(rb->evts);
    rb->evts = new_buf;
    rb->start = 0;
    rb->end = rb->size;
    rb->capacity = new_size;
}

struct MidiEvent ringbuf_pop(struct RingBuf *rb) {
    assert(rb != NULL);
    assert(!ringbuf_is_empty(rb));

    struct MidiEvent popped_event = rb->evts[rb->start];
    rb->start = (rb->start + 1) % rb->capacity;
    rb->size--;
    return popped_event;
}

struct MidiEvent ringubf_peek(struct RingBuf *rb) {
    assert(!ringbuf_is_empty(rb));
    return rb->evts[rb->start];
}

// TODO: Free the events in queue
void ringbuf_free(struct RingBuf *rb) {
    free(rb->evts);
    rb->evts = NULL;
    rb->capacity = rb->size = rb->start = rb->end = 0;
}

void ringbuf_print(struct RingBuf *rb) {
    LOG_INFO("--------------------Event Queue--------------------");

    for (int i = 0; i < rb->size; i++) {
        int ring_buf_idx = (rb->start + i) % rb->capacity;

        struct MidiEvent event = rb->evts[ring_buf_idx];
        if (event.type == SND_SEQ_EVENT_NOTEON) {
            printf("--> Note on : ");
        } else if (event.type == SND_SEQ_EVENT_NOTEOFF) {
            printf("-->. Note Off: ");
        }
        printf("type %d, pitch %d, velocity %d\n", event.type, event.note,
               event.velocity);
    }
    LOG_INFO("-----------------------------------------------------");
}
