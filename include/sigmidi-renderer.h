#ifndef SIGMIDI_RENDERER_H
#define SIGMIDI_RENDERER_H

#include <alsa/asoundlib.h>
#include <sigmidi.h>
#include <stdbool.h>
#include <stddef.h>

// Interface for renderer
void init_renderer(struct RendererOptions options);
void pre_drawing();
void begin_drawing();
void end_drawing();
void post_drawing();
bool window_should_close();
void draw_note(struct Note note);
// ... add more

#endif // SIGMIDI_RENDERER_H
