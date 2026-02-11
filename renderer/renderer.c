#include "sigmidi.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <raylib.h>
#include <sigmidi-renderer.h>

#define WHITE_PER_OCTAVE 7

const Color BG_COLOR = (Color){20, 20, 21, 255};
const Color PIANO_ROLL_WHITE = (Color){195, 195, 213, 255};
const Color PIANO_ROLL_BLACK = (Color){0, 0, 0, 255};
const Color FALLING_WHITE_NOTE_COLOR = (Color){187, 157, 189, 255};
const Color FALLING_BLACK_NOTE_COLOR = (Color){216, 100, 126, 255};
const Color MEASURE_LINE_COLOR = (Color){130, 130, 130, 127};
const Color OCTAVE_LINE_COLOR = (Color){130, 130, 130, 63};
const Color TEXT_COLOR = (Color){196, 130, 130, 255};

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

    int beats_per_measure;
    float bpm;
    int measure_len_ms;
    int measure_len_px;
};

static struct Layout layout;
static struct Player player;
static struct RendererOptions opt;
static struct AlsaClient client_list[10];
static struct AlsaClient sub_list[10];

void calc_layout() {
    layout.octave_count = opt.octave_count;
    layout.white_key_count = opt.octave_count * WHITE_PER_OCTAVE;
    layout.white_width = opt.width / layout.white_key_count;
    layout.white_height = opt.height / 8;

    layout.black_width = layout.white_width * 0.5;
    layout.black_height = opt.height / 12;

    layout.offset_y = opt.height * 7 / 8;
}

int calc_measure_len() {
    float ms_per_beat = (float)(60 * 1000) / player.bpm;
    return player.beats_per_measure * ms_per_beat;
}

void set_tempo(int t) {
    player.bpm = t;
    player.beats_per_measure = 4;
    player.measure_len_ms = calc_measure_len();
    player.measure_len_px = player.measure_len_ms * player.px_per_ms;
}

void resize_screen() {
    layout.white_width = GetScreenWidth() / layout.white_key_count;
    layout.white_height = GetScreenHeight() / 8;
    layout.black_width = layout.white_width * 0.5;
    layout.black_height = GetScreenHeight() / 12;
    layout.offset_y = GetScreenHeight() * 7 / 8;

    player.height_px = layout.offset_y;
    player.px_per_ms = (double)player.height_px / player.height_ms;
    set_tempo(player.bpm);
}

void toggle_fullscreen() {
    ToggleFullscreen();
    resize_screen();
}

void init_renderer(struct RendererOptions options) {
    opt = options;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(opt.width, opt.height, opt.title);
    SetTargetFPS(opt.fps);

    calc_layout();

    player.height_ms = 5000;
    player.height_px = layout.offset_y;
    player.px_per_ms = (double)player.height_px / player.height_ms;

    set_tempo(100);
}

void draw_octave_lines() {
    int y1 = 0;
    int y2 = player.height_px;
    for (int octave = 0; octave < layout.octave_count; octave++) {
        int base_white_idx = octave * WHITE_PER_OCTAVE;
        int x = base_white_idx * layout.white_width;
        DrawLine(x, y1, x, y2, OCTAVE_LINE_COLOR);
    }
}

void draw_measure_lines() {
    int x1 = 0;
    int x2 = GetScreenWidth();
    float curr_time_ms = GetTime() * 1000;
    float offset_ms = fmodf(curr_time_ms, player.measure_len_ms);
    float offset_px = offset_ms * player.px_per_ms;

    int n = player.height_ms / player.measure_len_ms;
    for (int i = 0; i <= n; i++) {
        int y = player.height_px - (i * player.measure_len_px) - offset_px;
        DrawLine(x1, y, x2, y, MEASURE_LINE_COLOR);
    }
}

void begin_drawing() {
    BeginDrawing();
    ClearBackground(BG_COLOR);
    draw_measure_lines();
    draw_octave_lines();
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

        DrawRectangle(x, y, w, h, PIANO_ROLL_WHITE);
        DrawRectangleLines(x, y, w, h, BG_COLOR);

        if (i % WHITE_PER_OCTAVE == 0) {
            // NOTE: octave labeling starts from C-1
            const char *text =
                TextFormat("C%d", i / WHITE_PER_OCTAVE + opt.octave_offset - 1);
            int font_s = 20;
            int font_x = x + 5;
            int font_y = GetScreenHeight() - font_s;
            DrawText(text, font_x, font_y, font_s, BG_COLOR);
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

            DrawRectangle(x, y, layout.black_width, layout.black_height,
                          PIANO_ROLL_BLACK);
        }
    }
}

void show_client_list() {
    list_seq_clients(client_list, 10);
    const char *lines[10];
    for (int i = 0; i < 10; i++) {
        if (strlen(client_list[i].name) == 0)
            continue;
        lines[i] = TextFormat("%3d: %s", client_list[i].id, client_list[i].name);
    };
    DrawText(TextJoin(lines, 10, "\n"), 0, 20, 20, TEXT_COLOR);
}

void show_sub_list() {
    list_subscribed_seq_clients(sub_list, 10);
    const char *lines[10];
    for (int i = 0; i < 10; i++) {
        if (strlen(sub_list[i].name) == 0)
            continue;
        lines[i] = TextFormat("%3d: %s", sub_list[i].id, sub_list[i].name);
    };
    DrawText(TextJoin(lines, 10, "\n"), 0, 20, 20, TEXT_COLOR);
}

void end_drawing() {
    draw_piano_roll();
    const char *status_str = TextFormat("Tempo: %d, Beats/Measure: %d", (int)player.bpm,
                                        player.beats_per_measure);
    DrawText(status_str, 0, 0, 20, TEXT_COLOR);
    if (IsKeyDown(KEY_L)) {
        show_client_list();
    } else if (IsKeyDown(KEY_S)) {
        show_sub_list();
    }
    EndDrawing();
}

bool window_should_close() {
    return WindowShouldClose();
}

Color get_velocity_color_tanh(Color base_color, unsigned char velocity) {
    if (!opt.velocity_based_color) {
        return base_color;
    }
    if (velocity == 0)
        return BLANK;
    /*
     * a: scaling coefficient i.e. how fast the color changes wrt velocity
     * b: velocity that will retain original color
     * x: input velocity
     * y: brightness factor scaled from -0.4 to +0.4
     */
    float a, b, x, y;
    a = 8;
    b = 70;
    x = velocity;
    y = (tanhf(a * (x - b) / 127)) * 0.4;
    return ColorBrightness(base_color, y);
}

void draw_note(struct Note note) {
    int x, y, w, h, duration;
    Color color;

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
        color = get_velocity_color_tanh(FALLING_BLACK_NOTE_COLOR, note.velocity);
    } else {
        x = (prev_white_note * layout.white_width);
        w = layout.white_width;
        color = get_velocity_color_tanh(FALLING_WHITE_NOTE_COLOR, note.velocity);
    }

    DrawRectangle(x, y, w, h, color);
    DrawRectangleLines(x, y, w, h, BG_COLOR);
}

void update_octave_count(int new_count) {
    opt.octave_count = new_count;
    calc_layout();
}

void pre_drawing() {
    if (IsWindowResized()) {
        resize_screen();
    }
    // Keyboard shortcuts
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        if (IsKeyPressed(KEY_EQUAL) && opt.octave_offset < 9) {
            opt.octave_offset++;
        }
        if (IsKeyPressed(KEY_MINUS) && opt.octave_offset > -1) {
            opt.octave_offset--;
        }
        if (IsKeyPressed(KEY_COMMA) && player.bpm > 10) {
            set_tempo(player.bpm - 5);
        }
        if (IsKeyPressed(KEY_PERIOD) && player.bpm < 300) {
            set_tempo(player.bpm + 5);
        }
    }

    if (IsKeyPressed(KEY_UP) && opt.octave_offset < 9) {
        update_octave_count(opt.octave_count + 1);
    }
    if (IsKeyPressed(KEY_DOWN) && opt.octave_offset > -1) {
        update_octave_count(opt.octave_count - 1);
    }
    if (IsKeyPressed(KEY_V)) {
        opt.velocity_based_color = !opt.velocity_based_color;
    }
    if (IsKeyPressed(KEY_F)) {
        toggle_fullscreen();
    }
    if (IsKeyDown(KEY_L)) {
        int client_idx = GetCharPressed() - '0' - 1;
        if (client_idx >= 0 && client_idx <= 9) {
            subscribe_to_a_sender(client_list[client_idx].name);
        }
    }
}

void post_drawing() {
}
