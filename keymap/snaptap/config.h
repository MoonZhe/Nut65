// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// The board reserves the user EEPROM datablock for RGB-record data and its own
// settings (keyboards/leku/nut65/config.h). Grow it by three bytes for the
// keymap's own settings: flags (Snap Tap, Knight Rider variant), Knight Rider
// speed, and modes (game mode). The board's eeconfig_init_user_datablock()
// only writes the RGB-record part, so these bytes are ours alone.
#undef EECONFIG_USER_DATA_SIZE
#define EECONFIG_USER_DATA_SIZE (EECONFIG_RGBREC_USE_SIZE + EECONFIG_CONFINFO_USE_SIZE + 3)
#define USER_FLAGS_EEPROM_ADDR ((uint8_t *)(EECONFIG_USER_DATABLOCK) + EECONFIG_RGBREC_USE_SIZE + EECONFIG_CONFINFO_USE_SIZE)
#define USER_KITT_SPEED_EEPROM_ADDR (USER_FLAGS_EEPROM_ADDR + 1)
#define USER_MODES_EEPROM_ADDR (USER_FLAGS_EEPROM_ADDR + 2)

// Lighting defaults, applied whenever the settings are reset (fresh EEPROM or
// a factory reset): keys light up only when pressed, in KITT red.
#undef RGB_MATRIX_DEFAULT_MODE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#undef RGB_MATRIX_DEFAULT_HUE
#define RGB_MATRIX_DEFAULT_HUE 0
#undef RGB_MATRIX_DEFAULT_SAT
#define RGB_MATRIX_DEFAULT_SAT 255
#undef RGB_MATRIX_DEFAULT_VAL
#define RGB_MATRIX_DEFAULT_VAL 255
#undef RGB_MATRIX_DEFAULT_SPD
#define RGB_MATRIX_DEFAULT_SPD 135
