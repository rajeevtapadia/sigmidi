#include <alloca.h>
#include <alsa/asoundlib.h>
#include <sigmidi.h>
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

void read_midi_events() {
    struct pollfd *pfds;
    int pfd_count;

    pfd_count = snd_seq_poll_descriptors_count(handle, POLLIN);
    pfds = (struct pollfd *)alloca(pfd_count * sizeof(struct pollfd));

    LOG_INFO("Poll file descriptors count: %d", pfd_count);
    LOG_INFO("Waiting for MIDI events on %d:%d", snd_seq_client_id(handle), local_port);

    while (1) {

        snd_seq_poll_descriptors(handle, pfds, pfd_count, POLLIN);

        if (poll(pfds, pfd_count, -1) < 0) {
            perror("poll error");
            continue;
        }

        if (pfds[0].revents & POLLIN) {
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

                snd_seq_free_event(event);
            }
        }
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

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage();
        return -1;
    }

    init_seqencer();

    subscribe_to_a_sender(argv[1]);

    read_midi_events();

    snd_seq_close(handle);
    handle = NULL;
    return 0;
}
