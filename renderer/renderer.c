#include "sigmidi.h"
#include <assert.h>
#include <limits.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

#define WHITE_PER_OCTAVE 7

struct Layout {
    int octave_count;

    int white_key_count;
    int white_width;
    int white_height;

    int black_width;
    int black_height;

    int offset_y;
};

struct Player {
    int height_ms;
    int height_px;
    double px_per_ms;
};

static struct Layout layout;
static struct Player player;
static struct RendererOptions opt;

void init_renderer(struct RendererOptions options) {
    opt = options;

    InitWindow(opt.width, opt.height, opt.title);
    SetTargetFPS(opt.fps);

    layout.octave_count = opt.octave_count;
    layout.white_key_count = opt.octave_count * WHITE_PER_OCTAVE;
    layout.white_width = opt.width / layout.white_key_count;
    layout.white_height = opt.height / 8;

    layout.black_width = layout.white_width * 0.5;
    layout.black_height = opt.height / 12;

    layout.offset_y = opt.height * 7 / 8;

    player.height_ms = 5000;
    player.height_px = layout.offset_y;
    player.px_per_ms = (double)player.height_px / player.height_ms;
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

        if (i % WHITE_PER_OCTAVE == 0) {
            const char *text =
                TextFormat("C%d", i / WHITE_PER_OCTAVE + opt.octave_offset);
            int font_s = 20;
            int font_x = x + 5;
            int font_y = GetScreenHeight() - font_s;
            DrawText(text, font_x, font_y, font_s, BLACK);
        }
    }

    static int black_key_pattern[] = {1, 1, 0, 1, 1, 1, 0};

    for (int octave = 0; octave < layout.octave_count; octave++) {
        int base_white_idx = octave * WHITE_PER_OCTAVE;
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
    int x, y, w, h, duration;

    // Calculate y and h
    // TODO: use the ALSA queue clock time
    double curr_time = GetTime() * 1000;
    duration = note.end - note.start;
    if (note.end == INT_MAX) {
        duration = curr_time - note.start;
    }
    y = player.height_px - ((curr_time - note.start) * player.px_per_ms);
    h = duration * player.px_per_ms;

    // Calculate x and w
    int base_white_idx = note.note / 12 * WHITE_PER_OCTAVE;
    base_white_idx -= opt.octave_offset * WHITE_PER_OCTAVE;
    int prev_white_note = base_white_idx + get_prev_white_idx(note.note);

    if (is_black_key(note.note)) {
        x = ((prev_white_note + 1) * layout.white_width) - (layout.black_width / 2);
        w = layout.black_width;
    } else {
        x = (prev_white_note * layout.white_width);
        w = layout.white_width;
    }

    DrawRectangle(x, y, w, h, RED);
    DrawRectangleLines(x, y, w, h, BLACK);
}
