// Copyright 2024 sdk66 (@sdk66)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SOCD keymap: the stock Nut65 keymap with the owner's VIA layout baked in
// as the default, plus:
//   - Fn+G: SOCD (last-input priority) on A/D. Logic follows
//     Pascal Getreuer's SOCD Cleaner (SOCD_CLEANER_LAST),
//     https://getreuer.info/posts/keyboards/socd-cleaner.
//   - 2008-style KITT scanner on the light bar, in six colours, as part of
//     the Fn+Insert light-bar cycle (after white): light floods in from both
//     ends, drains into the middle, floods back out and drains to the ends.
//     Fn+, / Fn+. = slower / faster. Plays one fast cycle at power-on.
//   - A typing speed meter on the light bar, after KITT in the Fn+Insert
//     cycle.
//   - Fn+Left Win: game mode. Left Win acts as Fn and Right Alt as Win, SOCD
//     turns on, and the Left Win key glows red.

#include QMK_KEYBOARD_H
#include "rgb_record/rgb_record.h"
#ifdef VIAL_PERSIST_EEPROM_ADDR
#    include "nvm_eeprom_via_internal.h"
#endif
#ifdef WAKE_DEBUG
#    include "wake_debug.h"
#endif

// QK_KB_0..29 are taken by the board's keycodes in keyboard.json.
#define SOCD_TOG QK_KB_30 // VIA CUSTOM(30)
#define KITT_SLOW QK_KB_31         // VIA CUSTOM(31)
#define KITT_FAST (QK_KB_31 + 1)  // VIA CUSTOM(32)
#define GAME_TOG  (QK_KB_31 + 2)  // VIA CUSTOM(33)

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
// Generated from nut65.layout.json by tools/gen_layers.py.
#include "layers.inc"
};

const uint16_t PROGMEM rgbrec_default_effects[RGBREC_CHANNEL_NUM][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        HS_GREEN, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, ________,   HS_GREEN,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, HS_GREEN,   HS_GREEN,   HS_GREEN, ________, ________, ________, ________, ________, ________, ________,   ________,           ________,  ________,
        ________,             ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, HS_GREEN,  ________,
        ________, ________,   ________,                       ________,                                         ________,   ________, HS_GREEN, HS_GREEN,  HS_GREEN,
        ________, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________
        ),

    [1] = LAYOUT(
        HS_GREEN, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, ________,   HS_GREEN,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, HS_GREEN,   HS_GREEN,   HS_GREEN, ________, ________, ________, ________, ________, ________, ________,   ________,           ________,  ________,
        ________,             ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, HS_GREEN,  ________,
        ________, ________,   ________,                       ________,                                         ________,   ________, HS_GREEN, HS_GREEN,  HS_GREEN,
        ________, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________
        ),

    [2] = LAYOUT(
        HS_GREEN, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, ________,   HS_GREEN,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________, 
        ________, HS_GREEN,   HS_GREEN,   HS_GREEN, ________, ________, ________, ________, ________, ________, ________,   ________,           ________,  ________,
        ________,             ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, HS_GREEN,  ________,
        ________, ________,   ________,                       ________,                                         ________,   ________, HS_GREEN, HS_GREEN,  HS_GREEN,
        ________, ________,   ________,   ________, ________, ________, ________, ________, ________, ________, ________,   ________, ________, ________,  ________
        ),
};
#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = {ENCODER_CCW_CW(HS_VOLU, HS_VOLD)},
    [1] = {ENCODER_CCW_CW(_______, _______)},
    [2] = {ENCODER_CCW_CW(_______, _______)},
    [3] = {ENCODER_CCW_CW(_______, _______)},
    [4] = {ENCODER_CCW_CW(_______, _______)}
};
#endif

// clang-format on

// ---------------------------------------------------------------------------
// Persistent flags: one byte at the end of the user EEPROM datablock (see
// config.h). High nibble is a marker so uninitialised EEPROM reads as "all off";
// bit 0 = SOCD, bits 1..3 = Knight Rider variant (0 = off).

#define USER_FLAGS_MARKER 0xA0
#define USER_FLAG_SOCD 0x01
#define USER_FLAGS_KITT_SHIFT 1

static bool    socd_enabled = false;
static uint8_t kitt_mode       = 0; // 0 = off, else 1..KITT_VARIANTS.

static void user_flags_save(void) {
    uint8_t flags = USER_FLAGS_MARKER | (kitt_mode << USER_FLAGS_KITT_SHIFT);
    if (socd_enabled) flags |= USER_FLAG_SOCD;
    eeprom_update_byte(USER_FLAGS_EEPROM_ADDR, flags);
}

// ---------------------------------------------------------------------------
// SOCD (last input priority with reactivation)

#define SOCD_FLASH_MS 1500
#define LED_INDEX_A 34
#define LED_INDEX_D 36

typedef struct {
    uint8_t keys[2]; // Opposing basic keycodes.
    bool    held[2]; // Physically held, independent of what is in the report.
} socd_pair_t;

static socd_pair_t socd_pairs[] = {
    {.keys = {KC_A, KC_D}},
};

static uint32_t socd_flash_timer = 0;

// Pressing a key releases its held opposite; releasing it re-presses the
// opposite if that is still held. The current key itself is left to normal
// processing.
static void process_socd(uint16_t keycode, keyrecord_t *record, socd_pair_t *pair) {
    if (keycode != pair->keys[0] && keycode != pair->keys[1]) {
        return;
    }
    const uint8_t i        = (keycode == pair->keys[1]);
    const uint8_t opposing = i ^ 1;

    pair->held[i] = record->event.pressed;

    if (socd_enabled && pair->held[opposing]) {
        if (record->event.pressed) {
            del_key(pair->keys[opposing]);
        } else {
            add_key(pair->keys[opposing]);
        }
    }
}

// ---------------------------------------------------------------------------
// Knight Rider light bar (2008 KITT style)
//
// A four-phase cycle, measured from a KI3000 scanner replica: light floods in
// from both ends to a full bar, drains into the middle, floods back out from the
// middle to a full bar, then drains out to the ends. Each moving edge is soft.
//
// The bar is 80 LEDs (71..150). The board treats it as 15 segments of uneven
// size (5 x 6 LEDs, then 10 x 5) and mirrors each segment's first LED onto the
// rest; we paint after that, per LED, so the effect is centred on the bar.
//
// The variants sit in the Fn+Insert (RL_MOD) cycle between the board's white
// (11) and off (12) modes. While one is showing, the board stays on mode 11
// underneath, so the light-bar brightness keys keep working.

#define KITT_LED_FIRST 71
#define KITT_LEDS      80
#define KITT_HALF      (KITT_LEDS / 2)
#define KITT_SOFT      8 // Width of the soft edge, in LEDs.
#define KITT_TRAVEL    ((KITT_HALF + KITT_SOFT) * 256) // Edge travel (LEDs * 256) across a whole half.

// The light in each half lies between two soft edges, measured in LEDs from
// the end of the bar: `hi` (towards the middle) and `lo` (towards the end).
// The cycle is four moves: hi floods in, lo drains in, lo floods back out,
// hi drains out. Durations are at normal speed (ms). A negative transition
// starts each move before the previous one ends, so the bar never quite fills
// or empties; a positive one adds a pause.
#define KITT_FLOOD_MS      1200
#define KITT_DRAIN_MS      1800
#define KITT_TRANSITION_MS (-300)

// Speed levels, slowest first, as a percentage of the durations above.
static const uint8_t kitt_speed_pct[] = {175, 130, 100, 78, 60};
#define KITT_SPEEDS        ARRAY_SIZE(kitt_speed_pct)
#define KITT_DEFAULT_SPEED 1
#define KITT_DEFAULT_VARIANT 1 // Red: the scanner is on by default.
#define KITT_RAINBOW  0xFF
#define KITT_BOARD_LAST_MODE 11 // Board's last colour mode before "off".

// Hue (QMK 0..255) per variant, in Fn+Insert order.
static const uint8_t kitt_hues[] = {
    0,            // Red, the classic.
    20,           // Amber.
    170,          // Blue.
    85,           // Green.
    200,          // Purple.
    KITT_RAINBOW, // Rainbow drifting along the bar.
};
#define KITT_VARIANTS ARRAY_SIZE(kitt_hues)

static uint8_t  kitt_speed = KITT_DEFAULT_SPEED;
static uint32_t kitt_timer;

// Speed-key feedback: the pressed key (Fn+, / Fn+.) blinks white once, or red
// three times when the speed is at its slowest / fastest.
#define LED_INDEX_COMMA     25
#define LED_INDEX_DOT       24
#define SPEED_BLINK_MS      150
#define SPEED_LIMIT_BLINKS  3
static uint8_t  speed_flash_led;
static bool     speed_flash_limit;
static uint32_t speed_flash_timer = 0;

// Light-bar modes that follow the KITT variants in the Fn+Insert cycle: the
// 1982 KITT scanner, then the WPM meter. Like KITT they paint over the board's
// white mode (11). Saved in the modes byte, so keep the values stable.
enum { BAR_NONE = 0, BAR_WPM, BAR_KITT82, BAR_LAST = BAR_KITT82 };
static uint8_t bar_mode = BAR_NONE;
static void    user_modes_save(void);

static void bar_set(uint8_t mode) {
    bar_mode   = mode;
    kitt_timer = timer_read32(); // The 1982 scanner starts from the left.
    user_modes_save();
}

// Board accessors added by patches/nut65-indicators-user-hook.patch.
uint8_t nut65_rl_mode(void);
uint8_t nut65_rl_brightness(void);
bool    nut65_rl_music(void);

static void kitt_set(uint8_t mode) {
    kitt_mode  = mode;
    kitt_timer = timer_read32();
    user_flags_save();
}

static void kitt_set_speed(uint8_t speed) {
    kitt_speed = speed;
    eeprom_update_byte(USER_KITT_SPEED_EEPROM_ADDR, speed);
}

#ifdef NUT65_BAR_SOLIDS_ONLY
// Trimmed Fn+Insert cycle: the board's solid colours (4 red .. 11 white), the
// KITT variants, the WPM meter, then off (12). Skips the board's rainbow and
// breathing modes (0..3).
#    define BAR_FIRST_SOLID 4
#    define BAR_OFF         12
void nut65_rl_set_mode(uint8_t mode);
#endif

// Fn+Insert. Returns false when we consumed the press.
static bool kitt_process_rl_mod(void) {
    if (nut65_rl_music()) {
        return true; // Board's press just leaves music mode.
    }
    if (kitt_mode) {
        if (kitt_mode < KITT_VARIANTS) {
            kitt_set(kitt_mode + 1);
            return false;
        }
        kitt_set(0);
        bar_set(BAR_KITT82);
        return false;
    }
    if (bar_mode == BAR_KITT82) {
        bar_set(BAR_WPM);
        return false;
    }
    if (bar_mode == BAR_WPM) {
        bar_set(BAR_NONE);
#ifdef NUT65_BAR_SOLIDS_ONLY
        nut65_rl_set_mode(BAR_OFF);
        return false;
#else
        return true; // Board advances 11 -> 12 (off).
#endif
    }
    if (nut65_rl_mode() == KITT_BOARD_LAST_MODE) {
        kitt_set(1);
        return false;
    }
#ifdef NUT65_BAR_SOLIDS_ONLY
    const uint8_t mode = nut65_rl_mode();
    nut65_rl_set_mode(mode >= BAR_FIRST_SOLID && mode < KITT_BOARD_LAST_MODE ? mode + 1 : BAR_FIRST_SOLID);
    return false;
#else
    return true;
#endif
}

// Smoothstep on 0..1024: slow start, fast middle, slow finish.
static int32_t kitt_ease(int32_t x) {
    return x * x * (3 * 1024 - 2 * x) / (1024 * 1024);
}

// Distance (LEDs * 256) an edge has moved for a move starting at `start` that
// lasts `dur`, at time `t`.
static int32_t kitt_move(int32_t t, int32_t start, int32_t dur) {
    int32_t x = (t - start) * 1024 / dur;
    x         = x < 0 ? 0 : x > 1024 ? 1024 : x;
    return kitt_ease(x) * KITT_TRAVEL / 1024;
}

// 0..255 for a soft edge: 0 at or below 0, 255 at KITT_SOFT LEDs and above.
static int32_t kitt_soft(int32_t v) {
    v /= KITT_SOFT;
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

// Where the soft edges are `elapsed_ms` into the cycle. The light bar and the
// per-key KITT effect both read the scanner through this, so they stay in step.
typedef struct {
    int32_t hi, lo; // Edges, LEDs * 256 from the end of the bar.
    int32_t period; // Cycle length, ms.
} kitt_edges_t;

static kitt_edges_t kitt_edges(int32_t pct, uint32_t elapsed_ms) {
    const int32_t fl  = KITT_FLOOD_MS * pct / 100;
    const int32_t dr  = KITT_DRAIN_MS * pct / 100;
    const int32_t gap = KITT_TRANSITION_MS * pct / 100;

    // Move start times: hi in, lo in, lo out, hi out.
    const int32_t s_hi_in  = 0;
    const int32_t s_lo_in  = s_hi_in + fl + gap;
    const int32_t s_lo_out = s_lo_in + dr + gap;
    const int32_t s_hi_out = s_lo_out + fl + gap;
    const int32_t period   = s_hi_out + dr + gap;
    const int32_t t        = elapsed_ms % period;

    // The previous cycle's hi-out can still be finishing when hi-in starts.
    return (kitt_edges_t){
        .hi     = KITT_TRAVEL - kitt_move(t + period, s_hi_out, dr) + kitt_move(t, s_hi_in, fl) - kitt_move(t, s_hi_out, dr),
        .lo     = -KITT_SOFT * 256 + kitt_move(t, s_lo_in, dr) - kitt_move(t, s_lo_out, fl),
        .period = period,
    };
}

// Colour of bar LED `led` (0..79) at `level` 0..255. Rainbow drifts with time.
static RGB kitt_color(uint8_t hue, uint8_t led, uint8_t level) {
    HSV hsv = {
        .h = hue == KITT_RAINBOW ? timer_read32() / 20 + led * 3 : hue,
        .s = 255,
        .v = level,
    };
    return hsv_to_rgb(hsv);
}

// 0..255 light at `d` LEDs from the end of the bar.
static uint8_t kitt_level(const kitt_edges_t *e, uint8_t d) {
    const int32_t pos = (int32_t)d * 256;
    return kitt_soft(e->hi - pos) * kitt_soft(pos - e->lo) / 255;
}

// Paints the scanner `elapsed_ms` into its cycle and returns the cycle length.
static int32_t kitt_render(uint8_t hue, int32_t pct, uint32_t elapsed_ms) {
    const kitt_edges_t e    = kitt_edges(pct, elapsed_ms);
    const uint16_t     peak = nut65_rl_brightness();
    for (uint8_t d = 0; d < KITT_HALF; d++) {
        const uint8_t level = (uint16_t)kitt_level(&e, d) * peak / 255;
        for (uint8_t side = 0; side < 2; side++) {
            const uint8_t led = side ? KITT_LEDS - 1 - d : d;
            const RGB     rgb = kitt_color(hue, led, level);
            rgb_matrix_set_color(KITT_LED_FIRST + led, rgb.r, rgb.g, rgb.b);
        }
    }
    return e.period;
}

// ---------------------------------------------------------------------------
// Boot animation: one KITT cycle at the fastest speed when the keyboard starts.

static bool     boot_anim_active = false;
static uint32_t boot_anim_timer;

static void boot_anim_render(void) {
    const uint8_t hue    = kitt_mode ? kitt_hues[kitt_mode - 1] : 0;
    const int32_t period = kitt_render(hue, kitt_speed_pct[KITT_SPEEDS - 1], timer_elapsed32(boot_anim_timer));
    if (timer_elapsed32(boot_anim_timer) >= (uint32_t)period) {
        boot_anim_active = false;
    }
}

// ---------------------------------------------------------------------------
// Per-key KITT sweep, the "KITT Sweep" RGB effect (rgb_matrix_user.inc). Each
// key takes the light of the bar LED above its horizontal position, using the
// bar's clock, speed and colour (red when the bar's scanner is off), so keys
// and bar move together, boot animation included. Brightness follows the key
// brightness setting.

static kitt_edges_t kitt_keys_frame;
static uint8_t      kitt_keys_hue;

void kitt_keys_begin(void) {
    kitt_keys_hue = kitt_mode ? kitt_hues[kitt_mode - 1] : 0;
    if (boot_anim_active) {
        kitt_keys_frame = kitt_edges(kitt_speed_pct[KITT_SPEEDS - 1], timer_elapsed32(boot_anim_timer));
    } else {
        kitt_keys_frame = kitt_edges(kitt_speed_pct[kitt_speed], timer_elapsed32(kitt_timer));
    }
}

// Bar LED (0..79) above key position `x` (0..224 across the board).
static uint8_t bar_led_at(uint8_t x) {
    return (uint16_t)x * (KITT_LEDS - 1) / 224;
}

RGB kitt_keys_color(uint8_t x, uint8_t brightness) {
    const uint8_t led = bar_led_at(x);
    const uint8_t d   = led < KITT_HALF ? led : KITT_LEDS - 1 - led;
    return kitt_color(kitt_keys_hue, led, (uint16_t)kitt_level(&kitt_keys_frame, d) * brightness / 255);
}

// KITT Reactive effect: the scanner's colour at key position `x` and `level`.
// Call kitt_keys_begin() first in the frame.
RGB kitt_keys_tint(uint8_t x, uint8_t level) {
    return kitt_color(kitt_keys_hue, bar_led_at(x), level);
}

// Bar Echo effect: what the light bar showed last frame above key position
// `x`, rescaled from the bar's brightness to the keys' `brightness`. Reads the
// LED driver's frame buffer, whose name changed in newer QMK (the Vial build
// sets NUT65_LED_BUFFER).
#ifndef NUT65_LED_BUFFER
#    define NUT65_LED_BUFFER rgb_matrix_ws2812_array
#    define NUT65_LED_TYPE   rgb_led_t
#endif
extern NUT65_LED_TYPE NUT65_LED_BUFFER[];

// ---------------------------------------------------------------------------
// 1982 KITT scanner, modelled on the TV car (measured from footage of it):
// eight red lamps; the light sweeps across in about 0.9 s, lingering briefly at
// each end, so a full left-right-left cycle is about 1.8 s. Each lamp comes on
// fully as the light reaches it, then cools like an incandescent bulb, so 4-5
// lamps glow at once and the trail wraps round the ends as the light turns.
// Drawn on the light bar (8 segments of 10 LEDs) and, as the "KITT 1982" key
// effect, across the keys using the lamp above each key. Follows the KITT
// speed keys; the default is one step slower than the TV car, 1.19 s per sweep.

#define K82_LAMPS    8
#define K82_PASS_MS  915 // One sweep at the middle speed; x1.3 = 1.19 s at the default.
#define K82_COOL_MS  550 // A lamp fades out over this after the light leaves.

static uint32_t k82_lit_at[K82_LAMPS]; // When each lamp last had the light on it.
static uint8_t  k82_level[K82_LAMPS];  // This frame's lamp brightness, 0..255.
static uint32_t k82_frame;             // timer_read32() of the last update.

void kitt82_begin(void) {
    const uint32_t now = timer_read32();
    if (now == k82_frame) return; // Bar and keys share one update per frame.
    k82_frame = now;

    const int32_t pass = (int32_t)K82_PASS_MS * kitt_speed_pct[kitt_speed] / 100;
    const int32_t t    = timer_elapsed32(kitt_timer) % (2 * pass);
    const int32_t x    = t < pass ? t : 2 * pass - t;
    const int32_t head = kitt_ease(x * 1024 / pass) * (K82_LAMPS * 256 - 1) / 1024; // Lamps * 256.
    k82_lit_at[head / 256] = now;

    for (uint8_t lamp = 0; lamp < K82_LAMPS; lamp++) {
        const uint32_t since = now - k82_lit_at[lamp];
        // Linear fade: lamps behind the light read about 100/80/60/40/20 %.
        k82_level[lamp] = since >= K82_COOL_MS ? 0 : 255 - since * 255 / K82_COOL_MS;
    }
}

// Bar LED `led` (0..79): its lamp's level, a little darker at the lamp dividers.
static uint8_t kitt82_led_level(uint8_t led) {
    const uint8_t level = k82_level[led * K82_LAMPS / KITT_LEDS];
    const uint8_t pos   = led % (KITT_LEDS / K82_LAMPS);
    return pos == 0 || pos == KITT_LEDS / K82_LAMPS - 1 ? level / 2 : level;
}

static void kitt82_render(void) {
    kitt82_begin();
    const uint16_t peak = nut65_rl_brightness();
    for (uint8_t led = 0; led < KITT_LEDS; led++) {
        rgb_matrix_set_color(KITT_LED_FIRST + led, (uint16_t)kitt82_led_level(led) * peak / 255, 0, 0);
    }
}

// KITT 1982 key effect: the lamp above key position `x`. Call kitt82_begin() first.
RGB kitt82_keys_color(uint8_t x, uint8_t brightness) {
    return (RGB){(uint16_t)k82_level[bar_led_at(x) * K82_LAMPS / KITT_LEDS] * brightness / 255, 0, 0};
}

RGB bar_echo_color(uint8_t x, uint8_t brightness) {
    const NUT65_LED_TYPE px  = NUT65_LED_BUFFER[KITT_LED_FIRST + bar_led_at(x)];
    const uint16_t       bar = MAX(nut65_rl_brightness(), 1);
    return (RGB){
        .r = MIN(255, px.r * brightness / bar),
        .g = MIN(255, px.g * brightness / bar),
        .b = MIN(255, px.b * brightness / bar),
    };
}

// Boot animation on the keys: the same sweep, whatever the key effect is.
static void boot_keys_render(void) {
    kitt_keys_begin();
    for (uint8_t i = 0; i < KITT_LED_FIRST; i++) {
        const RGB c = kitt_keys_color(g_led_config.point[i].x, rgb_matrix_get_val());
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
}

// ---------------------------------------------------------------------------
// Game mode: Left Win acts as Fn and Right Alt as Win, SOCD turns on
// (restored on exit), and the Left Win key glows red. Saved in its own byte
// with a 0xB0 marker: bit 0 = game mode, bit 1 = SOCD before game mode.

#define USER_MODES_MARKER     0xB0
#define USER_MODE_GAME        0x01
#define USER_MODE_SOCD_BEFORE 0x02
#define USER_MODE_BAR_SHIFT   2 // Bits 2..3: light-bar mode (BAR_*).
#define LED_INDEX_LGUI        8

static bool    game_mode        = false;
static bool    game_socd_before = false;
static uint8_t game_fn_layer    = 0; // Fn layer Left Win switched on, 0 = none.
static bool    game_ralt_as_gui = false;

static void user_modes_save(void) {
    uint8_t modes = USER_MODES_MARKER;
    if (game_mode) modes |= USER_MODE_GAME;
    if (game_socd_before) modes |= USER_MODE_SOCD_BEFORE;
    modes |= bar_mode << USER_MODE_BAR_SHIFT;
    eeprom_update_byte(USER_MODES_EEPROM_ADDR, modes);
}

static void game_mode_set(bool on) {
    if (on == game_mode) return;
    if (on) {
        game_socd_before = socd_enabled;
        socd_enabled  = true;
    } else {
        socd_enabled = game_socd_before;
    }
    game_mode = on;
    user_flags_save();
    user_modes_save();
}

// The Fn layer that goes with the current base layer (Windows 0 -> 1, Mac 2 -> 3).
static uint8_t game_fn_layer_for_base(void) {
    return get_highest_layer(default_layer_state) == 2 ? 3 : 1;
}

// Returns false when the event was handled here.
static bool game_mode_process(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_LGUI:
            if (record->event.pressed && game_mode) {
                game_fn_layer = game_fn_layer_for_base();
                layer_on(game_fn_layer);
                return false;
            }
            if (!record->event.pressed && game_fn_layer) {
                layer_off(game_fn_layer);
                game_fn_layer = 0;
                return false;
            }
            break;
        case KC_RALT:
            if (record->event.pressed && game_mode) {
                register_code(KC_RGUI);
                game_ralt_as_gui = true;
                return false;
            }
            if (!record->event.pressed && game_ralt_as_gui) {
                unregister_code(KC_RGUI);
                game_ralt_as_gui = false;
                return false;
            }
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Light-bar helpers for the WPM meter. Bar LEDs are numbered
// 0..79 from the left; `level` is 0..255 on top of the light-bar brightness.

static void bar_led(uint8_t i, RGB c, uint8_t level) {
    const uint16_t v = (uint16_t)level * nut65_rl_brightness() / 255;
    rgb_matrix_set_color(KITT_LED_FIRST + i, c.r * v / 255, c.g * v / 255, c.b * v / 255);
}

static RGB bar_hue(uint8_t hue) {
    return hsv_to_rgb((HSV){.h = hue, .s = 255, .v = 255});
}

// ---------------------------------------------------------------------------
// Typing speed meter: the bar fills outwards from the middle as WPM rises,
// green at the centre through yellow to red at the ends. A white peak marker
// holds at the highest point, then falls back.

#define WPM_FULL         120 // WPM that fills the bar.
#define WPM_EASE_STEP_MS 10  // The fill moves 1/8 of the way to its target per step.
#define WPM_PEAK_HOLD_MS 1500
#define WPM_PEAK_FALL_MS 40 // Per LED, once the hold is over.
#define WPM_IDLE_LEVEL   40 // Centre LEDs glow this much at 0 WPM, so the mode is visible.

static int32_t  wpm_level; // Fill per half, LEDs * 256.
static uint32_t wpm_ease_timer;
static uint8_t  wpm_peak; // LEDs from the middle.
static uint32_t wpm_peak_timer;

static void wpm_render(void) {
    const int32_t target = (int32_t)MIN(get_current_wpm(), WPM_FULL) * KITT_HALF * 256 / WPM_FULL;
    if (timer_elapsed32(wpm_ease_timer) > 500) {
        wpm_ease_timer = timer_read32(); // Mode just switched on.
    }
    while (timer_elapsed32(wpm_ease_timer) >= WPM_EASE_STEP_MS) {
        wpm_ease_timer += WPM_EASE_STEP_MS;
        wpm_level += (target - wpm_level) / 8;
    }

    const uint8_t lit = (wpm_level + 255) / 256;
    if (lit >= wpm_peak) {
        wpm_peak       = lit;
        wpm_peak_timer = timer_read32();
    } else if (timer_elapsed32(wpm_peak_timer) > WPM_PEAK_HOLD_MS + WPM_PEAK_FALL_MS) {
        wpm_peak--;
        wpm_peak_timer += WPM_PEAK_FALL_MS;
    }

    for (uint8_t d = 0; d < KITT_HALF; d++) {
        const int32_t fill  = wpm_level - (int32_t)d * 256; // Soft one-LED tip.
        uint8_t       level = fill <= 0 ? 0 : fill >= 256 ? 255 : fill;
        RGB           c     = bar_hue(85 - 85 * d / (KITT_HALF - 1));
        if (d == 0 && level < WPM_IDLE_LEVEL) {
            level = WPM_IDLE_LEVEL;
        }
        if (wpm_peak > lit && d == wpm_peak - 1) {
            c     = (RGB){255, 255, 255};
            level = 255;
        }
        bar_led(KITT_HALF - 1 - d, c, level);
        bar_led(KITT_HALF + d, c, level);
    }
}

// ---------------------------------------------------------------------------

// Wake fix. The board has two 5-minute sleep timers: the low-power idle
// timeout (keyboards/linker/wireless/lowpower.c) and the wireless connection
// timeout (wls/wls.c). When the second fires while the keyboard is already
// falling asleep, its sleep request stays pending and sends the keyboard
// straight back to sleep right after the next wake, so the first keypress only
// flashes the lights. Drop any such leftover request on wake; the board
// restarts its own timer right after this.
void lpwr_set_timeout_manual(bool enable);

void suspend_wakeup_init_user(void) {
    lpwr_set_timeout_manual(false);
#ifdef WAKE_DEBUG
    wake_debug_resume();
#endif
}

// ---------------------------------------------------------------------------

// Settings reset (fresh EEPROM or factory reset): scanner on in red, SOCD
// and game mode off, default speed.
void eeconfig_init_user(void) {
    socd_enabled  = false;
    kitt_mode        = KITT_DEFAULT_VARIANT;
    kitt_speed       = KITT_DEFAULT_SPEED;
    game_mode        = false;
    game_socd_before = false;
    bar_mode         = BAR_NONE;
    user_flags_save();
    eeprom_update_byte(USER_KITT_SPEED_EEPROM_ADDR, kitt_speed);
    user_modes_save();
}

#ifdef VIAL_PERSIST_EEPROM_ADDR
// Keep the Vial layout across flashes. vial-qmk stamps its EEPROM data with a
// random per-build ID and wipes it (keymap, macros, tap dance, combos, key
// overrides) whenever the stamp doesn't match. That is safe but means every
// flash loses your Vial changes. If the stored data was valid under a previous
// build and VIAL_PERSIST_VERSION (bump it whenever the EEPROM layout changes)
// still matches, re-stamp it for this build before VIA checks it.
void via_init_kb(void) {
    const uint8_t *magic = (const uint8_t *)VIA_EEPROM_MAGIC_ADDR;
    const bool     was_valid = eeprom_read_byte(magic) != 0xFF || eeprom_read_byte(magic + 1) != 0xFF || eeprom_read_byte(magic + 2) != 0xFF;
    if (was_valid && !via_eeprom_is_valid() && eeprom_read_byte(VIAL_PERSIST_EEPROM_ADDR) == VIAL_PERSIST_VERSION) {
        via_eeprom_set_valid(true);
    }
}
#endif

void keyboard_post_init_user(void) {
    uint8_t flags = eeprom_read_byte(USER_FLAGS_EEPROM_ADDR);
    if ((flags & 0xF0) == USER_FLAGS_MARKER) {
        socd_enabled = flags & USER_FLAG_SOCD;
        kitt_mode       = (flags >> USER_FLAGS_KITT_SHIFT) & 0x07;
        if (kitt_mode > KITT_VARIANTS) kitt_mode = 0;
    } else {
        kitt_mode = KITT_DEFAULT_VARIANT; // Never saved: start with the scanner on.
    }
    uint8_t speed = eeprom_read_byte(USER_KITT_SPEED_EEPROM_ADDR);
    kitt_speed    = speed < KITT_SPEEDS ? speed : KITT_DEFAULT_SPEED;
    uint8_t modes = eeprom_read_byte(USER_MODES_EEPROM_ADDR);
    if ((modes & 0xF0) == USER_MODES_MARKER) {
        game_mode        = modes & USER_MODE_GAME;
        game_socd_before = modes & USER_MODE_SOCD_BEFORE;
        bar_mode         = (modes >> USER_MODE_BAR_SHIFT) & 0x03;
        kitt_timer       = timer_read32();
        if (bar_mode > BAR_LAST || kitt_mode) bar_mode = BAR_NONE;
    }
#ifdef WAKE_DEBUG
    wake_debug_boot();
#endif
    boot_anim_active = true;
    boot_anim_timer  = timer_read32();

#ifdef VIAL_PERSIST_EEPROM_ADDR
    // VIA has checked or reset its data by now, so it matches this EEPROM layout.
    // Mark it so the next flash keeps it (see via_init_kb).
    eeprom_update_byte(VIAL_PERSIST_EEPROM_ADDR, VIAL_PERSIST_VERSION);
#endif

    // Our boot animation replaces the board's rainbow window on the keys.
    extern bool start_paoma_flag;
    start_paoma_flag = false;

    // A saved effect that this build doesn't have (e.g. after switching
    // between the full and trimmed builds) falls back to the default.
    if (rgb_matrix_get_mode() >= RGB_MATRIX_EFFECT_MAX) {
        rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
    }
#ifdef NUT65_BAR_SOLIDS_ONLY
    if (!kitt_mode && bar_mode == BAR_NONE && nut65_rl_mode() < BAR_FIRST_SOLID) {
        nut65_rl_set_mode(BAR_FIRST_SOLID);
    }
#endif
}

// The board's nut65.c already owns process_record_user, so hook in one step
// earlier. Runs before the board's RGB-record and Fn-row handling.
bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef WAKE_DEBUG
    wake_debug_key(record);
#endif
    switch (keycode) {
        case SOCD_TOG:
            if (record->event.pressed) {
                socd_enabled = !socd_enabled;
                user_flags_save();
                socd_flash_timer = timer_read32() | 1; // Never 0, which means "not flashing".
            }
            return false;
        case KITT_SLOW:
        case KITT_FAST:
            if (record->event.pressed) {
                const bool slower = keycode == KITT_SLOW;
                if (slower && kitt_speed > 0) {
                    kitt_set_speed(kitt_speed - 1);
                } else if (!slower && kitt_speed < KITT_SPEEDS - 1) {
                    kitt_set_speed(kitt_speed + 1);
                }
                speed_flash_led   = slower ? LED_INDEX_COMMA : LED_INDEX_DOT;
                speed_flash_limit = slower ? kitt_speed == 0 : kitt_speed == KITT_SPEEDS - 1;
                speed_flash_timer = timer_read32() | 1; // Never 0, which means "not flashing".
            }
            return false;
        case GAME_TOG:
            if (record->event.pressed) {
                game_mode_set(!game_mode);
            }
            return false;
        case RL_MOD:
            if (record->event.pressed) {
                return kitt_process_rl_mod();
            }
            return true;
    }
    if (!game_mode_process(keycode, record)) {
        return false;
    }
    for (uint8_t n = 0; n < ARRAY_SIZE(socd_pairs); n++) {
        process_socd(keycode, record, &socd_pairs[n]);
    }
    return true;
}

// Called at the end of the board's rgb_matrix_indicators_advanced_kb (needs
// patches/nut65-indicators-user-hook.patch), so anything painted here wins over
// the board's light-bar modes. Paints whole LEDs regardless of led_min/led_max;
// the last chunk of each frame is what gets flushed.
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (boot_anim_active) {
        boot_anim_render();
        boot_keys_render();
    } else if (!nut65_rl_music()) {
        if (kitt_mode) {
            kitt_render(kitt_hues[kitt_mode - 1], kitt_speed_pct[kitt_speed], timer_elapsed32(kitt_timer));
        } else if (bar_mode == BAR_KITT82) {
            kitt82_render();
        } else if (bar_mode == BAR_WPM) {
            wpm_render();
        }
    }
    if (game_mode) {
        rgb_matrix_set_color(LED_INDEX_LGUI, 0xFF, 0x00, 0x00);
    }
    // Speed key: one white blink, or red blinks at the slowest / fastest speed.
    if (speed_flash_timer) {
        const uint32_t e     = timer_elapsed32(speed_flash_timer);
        const uint8_t  times = speed_flash_limit ? SPEED_LIMIT_BLINKS : 1;
        if (e < (uint32_t)SPEED_BLINK_MS * (2 * times - 1)) {
            if ((e / SPEED_BLINK_MS) % 2 == 0) {
                if (speed_flash_limit) {
                    rgb_matrix_set_color(speed_flash_led, 0xFF, 0x00, 0x00);
                } else {
                    rgb_matrix_set_color(speed_flash_led, 0xFF, 0xFF, 0xFF);
                }
            } else {
                rgb_matrix_set_color(speed_flash_led, 0x00, 0x00, 0x00);
            }
        } else {
            speed_flash_timer = 0;
        }
    }
    // Flash A and D after a SOCD toggle: green = on, red = off.
    if (socd_flash_timer) {
        if (timer_elapsed32(socd_flash_timer) < SOCD_FLASH_MS) {
            uint8_t r = socd_enabled ? 0x00 : 0xFF;
            uint8_t g = socd_enabled ? 0xFF : 0x00;
            rgb_matrix_set_color(LED_INDEX_A, r, g, 0x00);
            rgb_matrix_set_color(LED_INDEX_D, r, g, 0x00);
        } else {
            socd_flash_timer = 0;
        }
    }
    return true;
}
