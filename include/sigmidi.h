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
    struct MidiEvent *evts;
    int start;
    int end;
    int size;
    int capacity;
};

struct MidiEvent {
    snd_seq_event_type_t type;
    unsigned char note;
    unsigned char velocity;
    // TODO: add appropriate time field here
};

struct RingBuf ringbuf_alloc();
void ringbuf_push(struct RingBuf *rb, struct MidiEvent evt);
bool ringbuf_is_full(struct RingBuf *rb);
bool ringbuf_is_empty(struct RingBuf *rb);
void ringbuf_resize(struct RingBuf *rb, int new_size);
struct MidiEvent ringbuf_pop(struct RingBuf *rb);
struct MidiEvent ringubf_peek(struct RingBuf *rb);
void ringbuf_free(struct RingBuf *rb);
void ringbuf_print(struct RingBuf *rb);

#endif // SIGMIDI_H
