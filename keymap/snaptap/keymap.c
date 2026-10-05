// Copyright 2024 sdk66 (@sdk66)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Snap Tap keymap: the stock Nut65 keymap with the owner's VIA layout baked in
// as the default, plus:
//   - Fn+G: last-input-priority SOCD on A/D (Razer "Snap Tap"). Logic follows
//     Pascal Getreuer's SOCD Cleaner (SOCD_CLEANER_LAST),
//     https://getreuer.info/posts/keyboards/socd-cleaner.
//   - 2008-style KITT scanner on the light bar, in six colours, as part of
//     the Fn+Insert light-bar cycle (after white): light floods in from both
//     ends, drains into the middle, floods back out and drains to the ends.
//     Fn+, / Fn+. = slower / faster.

#include QMK_KEYBOARD_H
#include "rgb_record/rgb_record.h"

// QK_KB_0..29 are taken by the board's keycodes in keyboard.json.
#define SNAP_TOG QK_KB_30 // VIA CUSTOM(30)
#define KITT_SLOW QK_KB_31         // VIA CUSTOM(31)
#define KITT_FAST (QK_KB_31 + 1)  // VIA CUSTOM(32)

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
// bit 0 = Snap Tap, bits 1..3 = Knight Rider variant (0 = off).

#define USER_FLAGS_MARKER 0xA0
#define USER_FLAG_SNAPTAP 0x01
#define USER_FLAGS_KITT_SHIFT 1

static bool    snaptap_enabled = false;
static uint8_t kitt_mode       = 0; // 0 = off, else 1..KITT_VARIANTS.

static void user_flags_save(void) {
    uint8_t flags = USER_FLAGS_MARKER | (kitt_mode << USER_FLAGS_KITT_SHIFT);
    if (snaptap_enabled) flags |= USER_FLAG_SNAPTAP;
    eeprom_update_byte(USER_FLAGS_EEPROM_ADDR, flags);
}

// ---------------------------------------------------------------------------
// Snap Tap (SOCD last input priority with reactivation)

#define SNAPTAP_FLASH_MS 1500
#define LED_INDEX_A 34
#define LED_INDEX_D 36

typedef struct {
    uint8_t keys[2]; // Opposing basic keycodes.
    bool    held[2]; // Physically held, independent of what is in the report.
} socd_pair_t;

static socd_pair_t socd_pairs[] = {
    {.keys = {KC_A, KC_D}},
};

static uint32_t snaptap_flash_timer = 0;

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

    if (snaptap_enabled && pair->held[opposing]) {
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
        return true; // Board advances 11 -> 12 (off).
    }
    if (nut65_rl_mode() == KITT_BOARD_LAST_MODE) {
        kitt_set(1);
        return false;
    }
    return true;
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

static void kitt_render(void) {
    const int32_t pct = kitt_speed_pct[kitt_speed];
    const int32_t fl  = KITT_FLOOD_MS * pct / 100;
    const int32_t dr  = KITT_DRAIN_MS * pct / 100;
    const int32_t gap = KITT_TRANSITION_MS * pct / 100;

    // Move start times: hi in, lo in, lo out, hi out.
    const int32_t s_hi_in  = 0;
    const int32_t s_lo_in  = s_hi_in + fl + gap;
    const int32_t s_lo_out = s_lo_in + dr + gap;
    const int32_t s_hi_out = s_lo_out + fl + gap;
    const int32_t period   = s_hi_out + dr + gap;
    const int32_t t        = timer_elapsed32(kitt_timer) % period;

    // The previous cycle's hi-out can still be finishing when hi-in starts.
    const int32_t hi = KITT_TRAVEL - kitt_move(t + period, s_hi_out, dr) + kitt_move(t, s_hi_in, fl) - kitt_move(t, s_hi_out, dr);
    const int32_t lo = -KITT_SOFT * 256 + kitt_move(t, s_lo_in, dr) - kitt_move(t, s_lo_out, fl);

    const uint8_t  hue  = kitt_hues[kitt_mode - 1];
    const uint8_t  tick = timer_read32() / 20;
    const uint16_t peak = nut65_rl_brightness();
    for (uint8_t d = 0; d < KITT_HALF; d++) {
        const int32_t pos   = (int32_t)d * 256;
        const uint8_t level = kitt_soft(hi - pos) * kitt_soft(pos - lo) / 255;
        for (uint8_t side = 0; side < 2; side++) {
            const uint8_t led = side ? KITT_LEDS - 1 - d : d;
            HSV hsv = {
                .h = hue == KITT_RAINBOW ? tick + led * 3 : hue,
                .s = 255,
                .v = (uint16_t)level * peak / 255,
            };
            RGB rgb = hsv_to_rgb(hsv);
            rgb_matrix_set_color(KITT_LED_FIRST + led, rgb.r, rgb.g, rgb.b);
        }
    }
}

// ---------------------------------------------------------------------------

void keyboard_post_init_user(void) {
    uint8_t flags = eeprom_read_byte(USER_FLAGS_EEPROM_ADDR);
    if ((flags & 0xF0) == USER_FLAGS_MARKER) {
        snaptap_enabled = flags & USER_FLAG_SNAPTAP;
        kitt_mode       = (flags >> USER_FLAGS_KITT_SHIFT) & 0x07;
        if (kitt_mode > KITT_VARIANTS) kitt_mode = 0;
    }
    uint8_t speed = eeprom_read_byte(USER_KITT_SPEED_EEPROM_ADDR);
    kitt_speed    = speed < KITT_SPEEDS ? speed : KITT_DEFAULT_SPEED;
}

// The board's nut65.c already owns process_record_user, so hook in one step
// earlier. Runs before the board's RGB-record and Fn-row handling.
bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case SNAP_TOG:
            if (record->event.pressed) {
                snaptap_enabled = !snaptap_enabled;
                user_flags_save();
                snaptap_flash_timer = timer_read32() | 1; // Never 0, which means "not flashing".
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
        case RL_MOD:
            if (record->event.pressed) {
                return kitt_process_rl_mod();
            }
            return true;
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
    if (kitt_mode && !nut65_rl_music()) {
        kitt_render();
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
    // Flash A and D after a Snap Tap toggle: green = on, red = off.
    if (snaptap_flash_timer) {
        if (timer_elapsed32(snaptap_flash_timer) < SNAPTAP_FLASH_MS) {
            uint8_t r = snaptap_enabled ? 0x00 : 0xFF;
            uint8_t g = snaptap_enabled ? 0xFF : 0x00;
            rgb_matrix_set_color(LED_INDEX_A, r, g, 0x00);
            rgb_matrix_set_color(LED_INDEX_D, r, g, 0x00);
        } else {
            snaptap_flash_timer = 0;
        }
    }
    return true;
}
