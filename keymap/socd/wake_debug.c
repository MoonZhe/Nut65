// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Temporary wake-from-sleep event log (build with WAKE_DEBUG=yes).
//
// The OEM wireless code calls dbg_wake_event() at each sleep/wake step (see
// patches/debug-wake-logging.patch). Events go into a RAM ring buffer, which
// survives the MCU's STOP sleep, and are read back over VIA raw HID with
// tools/wake_log.py once the keyboard is in wired mode.

#include QMK_KEYBOARD_H
#include "wake_debug.h"

uint8_t *md_getp_state(void);
uint8_t  wireless_get_current_devs(void);
// The wireless code redirects raw_hid_send() to this (keyboards/linker/wireless/md_raw.h).
void replaced_hid_send(uint8_t *data, uint8_t length);

#define WAKE_LOG_SIZE            128
#define WAKE_LOG_CMD_READ        0xF0
#define WAKE_LOG_CMD_CLEAR       0xF1
#define WAKE_LOG_KEYS_AFTER_WAKE 6
#define LPWR_STATE_WAKEUP        3 // lpwr_state_t LPWR_WAKEUP

// Event types (also decoded by tools/wake_log.py).
enum {
    WAKE_EV_STATE   = 1, // a = new lpwr state, extra = caller
    WAKE_EV_WAKECD  = 2, // a = wake source
    WAKE_EV_MANUAL  = 3, // a = enable, extra = caller (who asked to sleep)
    WAKE_EV_HOST    = 4, // a = host resumed (2.4G receiver), extra = lpwr state
    WAKE_EV_TIMEOUT = 5, // a = manual flag, extra = ms since last input
    WAKE_EV_MD      = 6, // a = new module state, extra = previous state
    WAKE_EV_KEY     = 7, // a = pressed, extra = row << 8 | col
    WAKE_EV_RESUME  = 8, // suspend_wakeup_init ran
    WAKE_EV_SUSPEND = 9, // suspend_power_down ran
    WAKE_EV_BOOT    = 10,
    WAKE_EV_LEDS    = 11, // a = lighting bits (see matrix_scan_user), extra = val << 24 | ms since last input
};

typedef struct __attribute__((packed)) {
    uint32_t time;
    uint8_t  type;
    uint8_t  a;
    uint8_t  md_state;
    uint8_t  devs;
    uint32_t extra;
} wake_log_entry_t; // 12 bytes

static wake_log_entry_t wake_log[WAKE_LOG_SIZE];
static uint16_t         wake_log_count = 0; // Events logged since boot or clear.
static uint8_t          keys_to_log    = 0;

void dbg_wake_event(uint8_t type, uint8_t a, uint32_t extra) {
    wake_log_entry_t *e = &wake_log[wake_log_count % WAKE_LOG_SIZE];
    e->time             = timer_read32();
    e->type             = type;
    e->a                = a;
    e->md_state         = *md_getp_state();
    e->devs             = wireless_get_current_devs();
    e->extra            = extra;
    wake_log_count++;
    if (type == WAKE_EV_STATE && a == LPWR_STATE_WAKEUP) {
        keys_to_log = WAKE_LOG_KEYS_AFTER_WAKE;
    }
}

void wake_debug_key(keyrecord_t *record) {
    if (keys_to_log) {
        keys_to_log--;
        dbg_wake_event(WAKE_EV_KEY, record->event.pressed, (uint32_t)record->event.key.row << 8 | record->event.key.col);
    }
}

void wake_debug_boot(void) {
    dbg_wake_event(WAKE_EV_BOOT, 0, 0);
}

// Lighting state, logged whenever it changes: bit 0 = RGB matrix enabled,
// bit 1 = RGB matrix suspended, bit 2 = LED power pin, bit 3 = LED boost pin.
void matrix_scan_user(void) {
    static uint8_t last = 0xFF;
    const uint8_t  now  = (rgb_matrix_is_enabled() ? 1 : 0) | (rgb_matrix_get_suspend_state() ? 2 : 0) | (gpio_read_pin(LED_POWER_EN_PIN) ? 4 : 0) | (gpio_read_pin(HS_LED_BOOSTING_PIN) ? 8 : 0);
    if (now != last) {
        last = now;
        dbg_wake_event(WAKE_EV_LEDS, now, (uint32_t)rgb_matrix_get_val() << 24 | (last_input_activity_elapsed() & 0xFFFFFF));
    }
}

void wake_debug_resume(void) {
    dbg_wake_event(WAKE_EV_RESUME, 0, 0);
}

void suspend_power_down_user(void) {
    dbg_wake_event(WAKE_EV_SUSPEND, 0, 0);
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

// Read: [0xF0, index] -> [0xF0, count_lo, count_hi, n, entry index, entry index + 1, now]
// where index counts from the oldest kept event. Clear: [0xF1].
bool dbg_wake_via_command(uint8_t *data, uint8_t length) {
    if (data[0] == WAKE_LOG_CMD_CLEAR) {
        wake_log_count = 0;
        replaced_hid_send(data, length);
        return true;
    }
    if (data[0] != WAKE_LOG_CMD_READ) {
        return false;
    }
    const uint16_t count = wake_log_count;
    const uint16_t kept  = count < WAKE_LOG_SIZE ? count : WAKE_LOG_SIZE;
    const uint16_t start = count < WAKE_LOG_SIZE ? 0 : count % WAKE_LOG_SIZE;
    const uint8_t  index = data[1];

    memset(data + 1, 0, length - 1);
    data[1] = count;
    data[2] = count >> 8;
    data[3] = kept;
    for (uint8_t n = 0; n < 2; n++) {
        if (index + n < kept) {
            memcpy(data + 4 + n * sizeof(wake_log_entry_t), &wake_log[(start + index + n) % WAKE_LOG_SIZE], sizeof(wake_log_entry_t));
        }
    }
    put32(data + 28, timer_read32());
    replaced_hid_send(data, length);
    return true;
}
