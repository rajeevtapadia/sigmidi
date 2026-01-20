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

void draw_note(struct Note note) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int bar_w = w / 127;
    int bar_h = bar_w;

    DrawRectangle(note.note *bar_w, h / 2, bar_w, bar_h, RED);

    DrawFPS(0, 0);
}
