#ifndef SIGMIDI_H
#define SIGMIDI_H

#include <alsa/asoundlib.h>
#include <stdbool.h>

#define LOG_INFO(fmt, ...) fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

extern snd_seq_t *handle;
extern int local_port;

// A dynamically sized queue that stores pointers to events
struct RingBuf {
    snd_seq_event_t **evts;
    int start;
    int end;
    int size;
    int capacity;
};

struct RingBuf ringbuf_alloc();
void ringbuf_push(struct RingBuf *rb, snd_seq_event_t *evt);
bool ringbuf_is_full(struct RingBuf *rb);
bool ringbuf_is_empty(struct RingBuf *rb);
void ringbuf_resize(struct RingBuf *rb, int new_size);
snd_seq_event_t *ringbuf_pop(struct RingBuf *rb);
snd_seq_event_t *ringubf_peek(struct RingBuf *rb);
void ringbuf_free(struct RingBuf *rb);
void ringbuf_print(struct RingBuf *rb);

#endif // SIGMIDI_H
