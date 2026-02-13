#ifndef SIGMIDI_H
#define SIGMIDI_H

#include <alsa/asoundlib.h>
#include <stdbool.h>

#define LOG_INFO(fmt, ...) fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

extern snd_seq_t *handle;
extern int local_port;
extern bool sustain_pedal;

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
    int sus_duration;
};

struct RendererOptions {
    int width;
    int height;
    const char *title;
    int fps;
    int octave_count;
    int octave_offset;
    bool velocity_based_color;
};

struct AlsaClient {
    int id;
    char name[64];
};


void list_seq_clients(struct AlsaClient *client_list, int size);
void list_subscribed_seq_clients(struct AlsaClient *client_list, int size);
void subscribe_to_a_sender(char *sender_str);
void unsubscribe_to_a_sender(char *sender_str);

#endif // SIGMIDI_H
