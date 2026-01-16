#include <alsa/asoundlib.h>
#include <assert.h>
#include <sigmidi.h>
#include <stdlib.h>

#define INIT_CAP 10

struct RingBuf ringbuf_alloc() {
    struct RingBuf rb;
    rb.evts = (snd_seq_event_t **)malloc(sizeof(snd_seq_event_t *) * INIT_CAP);
    rb.start = 0;
    rb.end = 0;
    rb.size = 0;
    rb.capacity = INIT_CAP;

    return rb;
}

void ringbuf_push(struct RingBuf *rb, snd_seq_event_t *evt) {
    assert(rb != NULL);

    if (ringbuf_is_full(rb)) {
        ringbuf_resize(rb, rb->capacity * 1.5);
    }

    rb->evts[rb->end] = evt;
    rb->end = (rb->end + 1) % rb->capacity;
    rb->size++;
}

bool ringbuf_is_full(struct RingBuf *rb) {
    return rb->size == rb->capacity;
}

bool ringbuf_is_empty(struct RingBuf *rb) {
    return rb->size == 0;
}

void ringbuf_resize(struct RingBuf *rb, int new_size) {
    assert(new_size > rb->capacity);

    snd_seq_event_t **new_buf = malloc(sizeof(snd_seq_event_t *) * new_size);

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

snd_seq_event_t *ringbuf_pop(struct RingBuf *rb) {
    assert(rb != NULL);

    if (ringbuf_is_empty(rb))
        return NULL;

    snd_seq_event_t *popped_event = rb->evts[rb->start];
    rb->start = (rb->start + 1) % rb->capacity;
    rb->size--;
    return popped_event;
}

snd_seq_event_t *ringubf_peek(struct RingBuf *rb) {
    if (ringbuf_is_empty(rb))
        return NULL;
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

        snd_seq_event_t *event = rb->evts[ring_buf_idx];
        if (event->type == SND_SEQ_EVENT_NOTEON) {
            printf("--> Note on : ");
        } else if (event->type == SND_SEQ_EVENT_NOTEOFF) {
            printf("--> Note Off: ");
        }
        printf("channel %d, pitch %d, velocity %d\n", event->data.note.channel,
               event->data.note.note, event->data.note.velocity);
    }
    LOG_INFO("-----------------------------------------------------");
}
