#include <alsa/asoundlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <sigmidi-renderer.h>
#include <sigmidi.h>
#include <stdlib.h>
#include <string.h>

#define RINGBUF_IMPLEMENTATION
#include <3dparty/generic-ringbuf.h>

snd_seq_t *handle;
int local_port;
int queue_id;
bool sustain_pedal = false;

void print_usage() {
    LOG_ERROR("Usage: sigmidi <client>:<port>");
}

void init_seqencer() {
    if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        LOG_ERROR("Error opening ALSA sequencer");
        exit(EXIT_FAILURE);
    }

    snd_seq_set_client_name(handle, "SigMidi Client");

    local_port = snd_seq_create_simple_port(
        handle, "Read Port", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    if (local_port < 0) {
        LOG_ERROR("Error creating sequencer port");
        exit(EXIT_FAILURE);
    }

    // Enable wall clock timestamping
    snd_seq_port_info_t *port_info;
    snd_seq_port_info_alloca(&port_info);
    snd_seq_get_port_info(handle, local_port, port_info);

    queue_id = snd_seq_alloc_queue(handle);

    snd_seq_port_info_set_timestamping(port_info, 1);
    snd_seq_port_info_set_timestamp_queue(port_info, queue_id);
    snd_seq_port_info_set_timestamp_real(port_info, 1);
    snd_seq_set_port_info(handle, local_port, port_info);

    snd_seq_start_queue(handle, queue_id, NULL);
    snd_seq_drain_output(handle);

    LOG_INFO("Client and Port created successfully: %d:%d", snd_seq_client_id(handle),
             local_port);
}

long long convert_alsa_real_time_to_ms(snd_seq_real_time_t time) {
    long long ms = time.tv_sec * 1000;
    ms += time.tv_nsec / 1000000;
    return ms;
}

static inline struct MidiEvent snd_seq_event_to_midi_event(snd_seq_event_t *alsa_evt) {
    // Check if wall clock timestamping is enabled
    assert(alsa_evt->flags & SND_SEQ_TIME_STAMP_REAL);

    struct MidiEvent midi_evt = {
        .type = alsa_evt->type,
        .note = alsa_evt->data.note.note,
        .velocity = alsa_evt->data.note.velocity,
        .time = convert_alsa_real_time_to_ms(alsa_evt->time.time),
    };

    if (alsa_evt->type == SND_SEQ_EVENT_NOTEON) {
        LOG_INFO("timestamp: %lld ms, velocity: %d", midi_evt.time, midi_evt.velocity);
    }
    return midi_evt;
}

void set_sustain_pedal(bool state, long long time, struct RingBuf *note_queue) {
    sustain_pedal = state;
    // Mute all the notes that are sustaining
    if (state == false) {
        if (ringbuf_is_empty(note_queue)) {
            return;
        }
        for (int i = 0; i < note_queue->size; i++) {
            int rb_idx = (note_queue->out + i) % note_queue->capacity;
            struct Note *note = *(struct Note **)(RINGBUF_AT(note_queue, rb_idx));

            if (time < (note->start + note->sus_duration) && note->sustain) {
                note->end = time;
                note->sustain = false;
            }
        }
    }
}

void read_midi_events(struct RingBuf *event_queue, struct RingBuf *note_queue) {
    snd_seq_event_t *event;
    while (snd_seq_event_input_pending(handle, 1) > 0) {
        if (snd_seq_event_input(handle, &event) < 0) {
            LOG_ERROR("Error in reading MIDI event");
        }

        struct MidiEvent midi_evt = snd_seq_event_to_midi_event(event);
        if (event->type == SND_SEQ_EVENT_CONTROLLER && event->data.control.param == 64) {
            LOG_INFO("sustain pedal - param: %d, value: %d", event->data.control.param,
                     event->data.control.value);

            set_sustain_pedal(event->data.control.value > 63, midi_evt.time, note_queue);
        }
        ringbuf_push(event_queue, &midi_evt);
        snd_seq_free_event(event);
    }
}

// Subscribe the local client to a sender using
// <client_id>:<port> or <client_name>:<port>
void subscribe_to_a_sender(char *sender_str) {
    snd_seq_addr_t sender_addr;
    if (snd_seq_parse_address(handle, &sender_addr, sender_str) < 0) {
        LOG_ERROR("Invalid client name or port: %s", sender_str);
        // TODO: better error handling
        exit(-1);
    }
    snd_seq_connect_from(handle, local_port, sender_addr.client, sender_addr.port);
    LOG_INFO("Subscribed to %s successfully!", sender_str);
}

void unsubscribe_to_a_sender(char *sender_str) {
    assert(sender_str);

    snd_seq_addr_t sender_addr;
    if (snd_seq_parse_address(handle, &sender_addr, sender_str) < 0) {
        LOG_ERROR("Invalid client name or port: %s", sender_str);
        exit(-1);
    }
    snd_seq_disconnect_from(handle, local_port, sender_addr.client, sender_addr.port);
    LOG_INFO("Unsubscribed to %s successfully!", sender_str);
}

long long alsa_time_now_ms() {
    snd_seq_queue_status_t *q_status;
    snd_seq_queue_status_alloca(&q_status);

    if (snd_seq_get_queue_status(handle, queue_id, q_status) < 0) {
        LOG_ERROR("Failed to get current time from ALSA queue");
        return 0.0;
    }

    const snd_seq_real_time_t *t = snd_seq_queue_status_get_real_time(q_status);
    return convert_alsa_real_time_to_ms(*t);
}

int calc_sustain_duration(struct Note n) {
    int note = n.note;
    int velocity = n.velocity;
    if (note < 21)
        note = 21;

    // Formula
    // Seconds = 35 * e^(-0.036 * (n - 21)) * (0.5 + (v / 254.0))
    double base_pitch_duration = 8.0 * exp(-0.036 * (note - 21));
    double velocity_multiplier = 0.5 + (velocity / 254.0);
    double duration_sec = base_pitch_duration * velocity_multiplier;

    return (int)(duration_sec * 1000.0);
}

// Process the ON/OFF midi events into struct Note with proper timestamping
void process_midi_events(struct RingBuf *event_queue, struct RingBuf *note_queue) {
    static struct Note *keys[255] = {0};

    while (!ringbuf_is_empty(event_queue)) {
        struct MidiEvent midi_evt;
        ringbuf_pop(event_queue, &midi_evt);

        if (midi_evt.type == SND_SEQ_EVENT_NOTEON && keys[midi_evt.note] == NULL) {
            struct Note *note = malloc(sizeof(struct Note));
            note->note = midi_evt.note;
            note->velocity = midi_evt.velocity;
            note->start = midi_evt.time;
            note->end = INT_MAX;

            ringbuf_push(note_queue, &note);
            keys[midi_evt.note] = note;
        } else if (midi_evt.type == SND_SEQ_EVENT_NOTEOFF &&
                   keys[midi_evt.note] != NULL) {
            struct Note *note = keys[midi_evt.note];
            if (sustain_pedal) {
                note->end = midi_evt.time;
                note->sus_duration = calc_sustain_duration(*note);
                note->sustain = true;
            } else {
                note->end = midi_evt.time;
                note->sus_duration = 0;
                note->sustain = false;
            }
            keys[midi_evt.note] = NULL;
        }
    }
}

void gc_note_queue(struct RingBuf *note_queue) {
    if (ringbuf_is_empty(note_queue))
        return;

    long long time_now_ms = alsa_time_now_ms();

    while (!ringbuf_is_empty(note_queue)) {
        struct Note *item;
        ringubf_peek(note_queue, &item);
        // TODO: Hardcoded value BAD!!
        if (item->end > (time_now_ms - 5000)) {
            break;
        }

        ringbuf_pop(note_queue, NULL);
        free(item);
    }
}

void event_loop() {
    struct RingBuf event_queue = ringbuf_alloc(sizeof(struct MidiEvent));
    struct RingBuf note_queue = ringbuf_alloc(sizeof(struct Note *));

    // Start the event loop
    while (!window_should_close()) {
        read_midi_events(&event_queue, &note_queue);
        process_midi_events(&event_queue, &note_queue);

        pre_drawing();
        begin_drawing();

        if (!ringbuf_is_empty(&note_queue)) {
            for (int i = 0; i < note_queue.size; i++) {
                int rb_idx = (note_queue.out + i) % note_queue.capacity;
                draw_note(**(struct Note **)(RINGBUF_AT(&note_queue, rb_idx)));
            }
        }

        end_drawing();
        post_drawing();

        gc_note_queue(&note_queue);
    }

    ringbuf_free(&event_queue);
    ringbuf_free(&note_queue);
}

void list_seq_clients(struct AlsaClient *client_list, int size) {
    assert(size > 0);
    assert(handle != NULL);
    assert(client_list);
    memset(client_list, 0, sizeof(struct AlsaClient) * size);

    snd_seq_client_info_t *cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    int i = 0;
    while (i < size && snd_seq_query_next_client(handle, cinfo) >= 0) {
        int id = snd_seq_client_info_get_client(cinfo);
        if (id != snd_seq_client_id(handle)) {
            client_list[i].id = id;
            strcpy(client_list[i].name, snd_seq_client_info_get_name(cinfo));
            i++;
        }
    }
}

int get_seq_client_name(int client_id, char buf[64]) {
    snd_seq_client_info_t *cinfo;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, client_id);

    if (snd_seq_get_any_client_info(handle, client_id, cinfo) < 0)
        return -1;

    const char *name = snd_seq_client_info_get_name(cinfo);
    if (!name)
        return -1;

    snprintf(buf, 64, "%s", name);
    return 0;
}

void list_subscribed_seq_clients(struct AlsaClient *client_list, int size) {
    assert(size > 0);
    assert(handle != NULL);
    assert(client_list);
    memset(client_list, 0, sizeof(struct AlsaClient) * size);

    snd_seq_query_subscribe_t *query;
    snd_seq_query_subscribe_alloca(&query);

    snd_seq_addr_t local_addr = {
        .client = snd_seq_client_id(handle),
        .port = local_port,
    };

    snd_seq_query_subscribe_set_root(query, &local_addr);
    snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(query, 0);

    int i = 0;
    while (i < size && snd_seq_query_port_subscribers(handle, query) == 0) {
        const snd_seq_addr_t *subscriber = snd_seq_query_subscribe_get_addr(query);
        client_list[i].id = subscriber->client;
        get_seq_client_name(subscriber->client, client_list[i].name);
        i++;
        snd_seq_query_subscribe_set_index(query, i);
    }
}

int main(int argc, char **argv) {
    // if (argc != 2) {
    //     print_usage();
    //     return -1;
    // }

    init_seqencer();
    if (argc == 2) {
        subscribe_to_a_sender(argv[1]);
    }

    struct RendererOptions opt = {
        .width = 1600,
        .height = 900,
        .title = "SigMidi",
        .fps = 60,
        .octave_count = 5,
        .octave_offset = 3,
        .velocity_based_color = true,
    };
    init_renderer(opt);

    event_loop();

    snd_seq_close(handle);
    handle = NULL;
    return 0;
}
