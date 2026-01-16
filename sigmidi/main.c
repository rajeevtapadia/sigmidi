#include <alloca.h>
#include <alsa/asoundlib.h>
#include <sigmidi-renderer.h>
#include <sigmidi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>

snd_seq_t *handle;
int local_port;

void print_usage() {
    LOG_ERROR("Usage: sigmidi <client>:<port>");
}

void init_seqencer() {
    if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
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

    LOG_INFO("Client and Port created successfully: %d:%d", snd_seq_client_id(handle),
             local_port);
}

void read_midi_events(struct RingBuf *event_queue) {
    snd_seq_event_t *event;
    while (snd_seq_event_input_pending(handle, 1) > 0) {
        if (snd_seq_event_input(handle, &event) < 0) {
            LOG_ERROR("Error in reading MIDI event");
        }

        if (event->type == SND_SEQ_EVENT_NOTEON) {
            printf("Note on : ");

        } else if (event->type == SND_SEQ_EVENT_NOTEOFF) {

            printf("Note Off: ");
        }
        printf("channel %d, pitch %d, velocity %d\n", event->data.note.channel,
               event->data.note.note, event->data.note.velocity);

        ringbuf_push(event_queue, event);
        // snd_seq_free_event(event);
    }
}

// Subscribe the local client to a sender using
// <client_id>:<port> or <client_name>:<port>
void subscribe_to_a_sender(char *sender_str) {
    snd_seq_addr_t sender_addr;
    if (snd_seq_parse_address(handle, &sender_addr, sender_str) < 0) {
        LOG_ERROR("Invalid port address: %s", sender_str);
        exit(-1);
    }
    snd_seq_connect_from(handle, local_port, sender_addr.client, sender_addr.port);
}

void event_loop() {
    struct RingBuf event_queue = ringbuf_alloc();

    // Start the event loop
    while (!window_should_close()) {
        read_midi_events(&event_queue);
        ringbuf_print(&event_queue);

        begin_drawing();
        for (int i = 0; i < event_queue.size; i++) {
            draw_note(event_queue.evts[i]);
        }
        end_drawing();
    }

    ringbuf_free(&event_queue);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage();
        return -1;
    }

    init_seqencer();
    subscribe_to_a_sender(argv[1]);

    init_renderer(800, 600, "SigMidi");

    event_loop();

    snd_seq_close(handle);
    handle = NULL;
    return 0;
}
