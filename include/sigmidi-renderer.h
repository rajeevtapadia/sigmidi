#ifndef SIGMIDI_RENDERER_H
#define SIGMIDI_RENDERER_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <alsa/asoundlib.h>

// Interface for renderer

struct Note {
    char name;
    char octave;
    size_t velocity;
    time_t start;
    time_t end;
};

void init_renderer(int width, int height, const char *title);
void begin_drawing();
void end_drawing();
bool window_should_close();
void draw_note(snd_seq_event_t *event);
// ... add more

#endif // SIGMIDI_RENDERER_H
