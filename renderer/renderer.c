#include <alsa/asoundlib.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

void init_renderer(int width, int height, const char *title) {
    InitWindow(width, height, title);
    SetTargetFPS(60);
}

void begin_drawing() {
    BeginDrawing();
}

void end_drawing() {
    EndDrawing();
}

bool window_should_close() {
    return WindowShouldClose();
}

void draw_note(snd_seq_event_t *event) {
    if (event->type == SND_SEQ_EVENT_NOTEON) {
        DrawRectangle(100, 100, 200, 200, RED);
    } else if (event->type == SND_SEQ_EVENT_NOTEOFF) {
        ClearBackground(BLACK);
    }
}
