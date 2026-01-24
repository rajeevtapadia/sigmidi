#include "sigmidi.h"
#include <assert.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

struct Layout {
    int octave_count;

    int white_per_octave;
    int white_key_count;
    int white_width;
    int white_height;

    int black_width;
    int black_height;

    int offset_y;
};

static struct Layout layout;

void init_renderer(int width, int height, const char *title, int octave_count) {
    InitWindow(width, height, title);
    SetTargetFPS(60);

    layout.octave_count = octave_count;
    layout.white_per_octave = 7;
    layout.white_key_count = octave_count * layout.white_per_octave;
    layout.white_width = width / layout.white_key_count;
    layout.white_height = height / 8;

    layout.black_width = layout.white_width * 0.5;
    layout.black_height = height / 12;

    layout.offset_y = height - (height / 8);
}

void begin_drawing() {
    BeginDrawing();
    ClearBackground(BLACK);
}

static void draw_piano_roll() {
    int y = layout.offset_y;

    for (int i = 0; i <= layout.white_key_count; i++) {
        int x = i * layout.white_width;
        int w = layout.white_width - 1;
        int h = layout.white_height;

        DrawRectangle(x, y, w, h, RAYWHITE);
        DrawRectangleLines(x, y, w, h, BLACK);
    }

    static int black_key_pattern[] = {1, 1, 0, 1, 1, 1, 0};

    for (int octave = 0; octave < layout.octave_count; octave++) {
        int base_white_idx = octave * layout.white_per_octave;
        for (int key = 0; key < 7; key++) {
            if (!black_key_pattern[key])
                continue;
            int white_idx = base_white_idx + key;
            int x = (white_idx + 1) * layout.white_width - (layout.black_width / 2);

            DrawRectangle(x, y, layout.black_width, layout.black_height, BLACK);
        }
    }
}

void end_drawing() {
    draw_piano_roll();
    DrawFPS(0, 0);
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

    DrawRectangle(note.note * bar_w, h / 2, bar_w, bar_h, RED);
}
