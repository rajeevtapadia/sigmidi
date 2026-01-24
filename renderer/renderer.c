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

static bool is_black_key(unsigned char note) {
    static const bool black_lut[12] = {
        0, // C
        1, // C#
        0, // D
        1, // D#
        0, // E
        0, // F
        1, // F#
        0, // G
        1, // G#
        0, // A
        1, // A#
        0  // B
    };
    return black_lut[note % 12];
}

static int get_prev_white_idx(unsigned char note) {
    static const int prev_white_idx_lut[12] = {
        0, // C
        0, // C#
        1, // D
        1, // D#
        2, // E
        3, // F
        3, // F#
        4, // G
        4, // G#
        5, // A
        5, // A#
        6  // B
    };
    return prev_white_idx_lut[note % 12];
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
    int x;
    int w = is_black_key(note.note) ? layout.black_width : layout.white_width;
    // TODO: Set y and h as per timestamp
    int y = GetScreenHeight() / 2;
    int h = 300;

    int base_white_idx = note.note / 12 * 7;
    int prev_white_note = base_white_idx + get_prev_white_idx(note.note);

    if (is_black_key(note.note)) {
        x = ((prev_white_note + 1) * layout.white_width) - (layout.black_width / 2);
    } else {
        x = (prev_white_note * layout.white_width);
    }

    DrawRectangle(x, y, w, h, RED);
    DrawRectangleLines(x, y, w, h, BLACK);
}
