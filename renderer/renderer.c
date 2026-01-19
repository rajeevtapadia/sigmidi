#include "sigmidi.h"
#include <assert.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

void init_renderer(int width, int height, const char *title) {
    InitWindow(width, height, title);
    SetTargetFPS(60);
}

void begin_drawing() {
    BeginDrawing();
    ClearBackground(BLACK);
}

void end_drawing() {
    EndDrawing();
}

bool window_should_close() {
    return WindowShouldClose();
}

void draw_note(struct MidiEvent event) {
    if (event.type == SND_SEQ_EVENT_NOTEON) {
        DrawRectangle(100, 100, 200, 200, RED);
    } else if (event.type == SND_SEQ_EVENT_NOTEOFF) {
        // ClearBackground(BLACK);
    }
    DrawFPS(0, 0);
}
