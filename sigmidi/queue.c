#include <alsa/asoundlib.h>
#include <assert.h>
#include <sigmidi.h>
#include <stdbool.h>
#include <stdlib.h>

#define INIT_CAP 10

// A dynamically sized queue that stores pointers to events
struct Queue {
    snd_seq_event_t **evts;
    int start;
    int end;
    int size;
    int capacity;
};

void queue_push(struct Queue *q, snd_seq_event_t *evt);
bool is_queue_full(struct Queue *q);
void queue_resize(struct Queue *q, int new_size);
snd_seq_event_t *queue_pop(struct Queue *q);
snd_seq_event_t *queue_peek(struct Queue *q);
void free_queue(struct Queue *q);
void print_queue(struct Queue *q);

struct Queue alloc_queue() {
    struct Queue q;
    q.start = 0;
    q.end = 0;
    q.evts = (snd_seq_event_t **)malloc(sizeof(snd_seq_event_t *) * INIT_CAP);
    q.size = 0;
    q.capacity = INIT_CAP;

    return q;
}

void queue_push(struct Queue *q, snd_seq_event_t *evt) {
    assert(q != NULL);

    if (is_queue_full(q)) {
        queue_resize(q, q->capacity * 1.5);
    }

    q->evts[q->end] = evt;
    q->end = (q->end + 1) % q->capacity;
    q->size++;
}

bool is_queue_full(struct Queue *q) {
    return q->size == q->capacity;
}

bool is_queue_empty(struct Queue *q) {
    return q->size == 0;
}

void queue_resize(struct Queue *q, int new_size) {
    assert(new_size > q->capacity);

    snd_seq_event_t **new_buf = malloc(sizeof(snd_seq_event_t *) * new_size);

    for (int i = 0; i < q->size; i++) {
        int ring_buf_idx = (q->start + i) % q->capacity;
        new_buf[i] = q->evts[ring_buf_idx];
    }

    free(q->evts);
    q->evts = new_buf;
    q->start = 0;
    q->end = q->size;
    q->capacity = new_size;
}

snd_seq_event_t *queue_pop(struct Queue *q) {
    assert(q != NULL);

    if (is_queue_empty(q))
        return NULL;

    snd_seq_event_t *popped_event = q->evts[q->start];
    q->start = (q->start + 1) % q->capacity;
    q->size--;
    return popped_event;
}

snd_seq_event_t *queue_peek(struct Queue *q) {
    if (is_queue_empty(q))
        return NULL;
    return q->evts[q->start];
}

void free_queue(struct Queue *q) {
    free(q->evts);
    q->evts = NULL;
    q->capacity = q->size = q->start = q->end = 0;
}

void print_queue(struct Queue *q) {
    LOG_INFO("--------------------Event Queue--------------------");

    for (int i = 0; i < q->size; i++) {
        int ring_buf_idx = (q->start + i) % q->capacity;

        snd_seq_event_t *event = q->evts[ring_buf_idx];
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
