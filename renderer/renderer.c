#include "sigmidi.h"
#include <assert.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

static int white_per_octave;
static int white_key_count;
static int white_width;
static int white_height;
static int black_width;
static int black_height;
static int octave_count;

void init_renderer(int width, int height, const char *title, int octaves) {
    InitWindow(width, height, title);
    SetTargetFPS(60);

    octave_count = octaves;
    white_per_octave = 7;
    white_key_count = octave_count * white_per_octave;
    white_width = width / white_key_count;
    white_height = height / 8;

    black_width = white_width * 0.5;
    black_height = height / 12;
}

void begin_drawing() {
    BeginDrawing();
    ClearBackground(BLACK);
}

static void draw_piano_roll() {
    int h = GetScreenHeight();
    int y = h - (h / 8);

    for (int i = 0; i <= white_key_count; i++) {
        DrawRectangle(i * white_width, y, white_width - 1, white_height, RAYWHITE);
        DrawRectangleLines(i * white_width, y, white_width - 1, white_height, BLACK);
    }

    static int black_key_pattern[] = {1, 1, 0, 1, 1, 1, 0};

    for (int octave = 0; octave < octave_count; octave++) {
        int base_white_idx = octave * white_per_octave;
        for (int key = 0; key < 7; key++) {
            if (!black_key_pattern[key])
                continue;
            int white_idx = base_white_idx + key;
            int x = (white_idx + 1) * white_width - (black_width / 2);

            DrawRectangle(x, y, black_width, black_height, BLACK);
        }
    }
}

void end_drawing() {
    draw_piano_roll();
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

    DrawFPS(0, 0);
}
