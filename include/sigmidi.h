#ifndef SIGMIDI_H
#define SIGMIDI_H

#include <alsa/asoundlib.h>
#include <stdbool.h>

#define LOG_INFO(fmt, ...) fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

extern snd_seq_t *handle;
extern int local_port;

struct MidiEvent {
    snd_seq_event_type_t type;
    unsigned char note;
    unsigned char velocity;
    long long time;
};

struct Note {
    unsigned char note;
    unsigned char velocity;
    int start;
    int end;
};

#endif // SIGMIDI_H
