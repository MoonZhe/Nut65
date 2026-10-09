// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The board's own custom effects, plus three that tie the keys to the light
// bar (helpers in keymap.c):
//   KITT Sweep     the light-bar scanner carried across the keys, in step.
//   KITT Reactive  keys stay dark; each press sends a soft streak along its
//                  row in the scanner's colour.
//   Bar Echo       each key shows what the light bar shows above it.
//   KITT 1982      the original red scanner across the keys, in step with
//                  the light bar's 1982 mode.
// This file replaces the board's rgb_matrix_user.inc, so include that first.

#include "../../rgb_matrix_user.inc"

RGB_MATRIX_EFFECT(KITT_SWEEP)
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
RGB_MATRIX_EFFECT(KITT_REACTIVE)
#endif
RGB_MATRIX_EFFECT(BAR_ECHO)
RGB_MATRIX_EFFECT(KITT_1982)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#    define NUT65_KEY_LEDS 71 // LEDs 71.. are the light bar, painted by the keymap.

void kitt_keys_begin(void);
RGB  kitt_keys_color(uint8_t x, uint8_t brightness);
RGB  kitt_keys_tint(uint8_t x, uint8_t level);
RGB  bar_echo_color(uint8_t x, uint8_t brightness);
void kitt82_begin(void);
RGB  kitt82_keys_color(uint8_t x, uint8_t brightness);

static bool KITT_SWEEP(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    kitt_keys_begin();
    for (uint8_t i = led_min; i < led_max && i < NUT65_KEY_LEDS; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const RGB c = kitt_keys_color(g_led_config.point[i].x, rgb_matrix_get_val());
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#    ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
// Positions are in LED units: 0..224 across, rows about 13 apart.
#        define KITT_REACTIVE_ROW     6   // Keys within this of the pressed key's y count as its row.
#        define KITT_REACTIVE_TAIL    40  // Light fades out over this distance behind the moving front.
#        define KITT_REACTIVE_EDGE    8   // Soft leading edge.
#        define KITT_REACTIVE_LIFE_MS 900 // The whole streak fades out over this.

static bool KITT_REACTIVE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    kitt_keys_begin();
    // Front speed in LED units per 256 ms; follows the effect speed setting.
    const uint32_t pace = 96 + rgb_matrix_get_speed();
    for (uint8_t i = led_min; i < led_max && i < NUT65_KEY_LEDS; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        int32_t level = 0;
        for (uint8_t j = 0; j < g_last_hit_tracker.count; j++) {
            const uint16_t tick = g_last_hit_tracker.tick[j];
            if (tick >= KITT_REACTIVE_LIFE_MS || abs(g_led_config.point[i].y - g_last_hit_tracker.y[j]) > KITT_REACTIVE_ROW) {
                continue;
            }
            const int32_t dx    = abs(g_led_config.point[i].x - g_last_hit_tracker.x[j]);
            const int32_t front = tick * pace / 256;
            int32_t       l     = dx > front ? 255 - (dx - front) * 255 / KITT_REACTIVE_EDGE : 255 - (front - dx) * 255 / KITT_REACTIVE_TAIL;
            l                   = l * (KITT_REACTIVE_LIFE_MS - tick) / KITT_REACTIVE_LIFE_MS;
            level               = MAX(level, l);
        }
        const RGB c = kitt_keys_tint(g_led_config.point[i].x, (uint16_t)MAX(level, 0) * rgb_matrix_get_val() / 255);
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}
#    endif

static bool BAR_ECHO(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    for (uint8_t i = led_min; i < led_max && i < NUT65_KEY_LEDS; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const RGB c = bar_echo_color(g_led_config.point[i].x, rgb_matrix_get_val());
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static bool KITT_1982(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    kitt82_begin();
    for (uint8_t i = led_min; i < led_max && i < NUT65_KEY_LEDS; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const RGB c = kitt82_keys_color(g_led_config.point[i].x, rgb_matrix_get_val());
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
