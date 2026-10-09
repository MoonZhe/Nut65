// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// Temporary wake-from-sleep event log; see wake_debug.c.
void wake_debug_key(keyrecord_t *record);
void wake_debug_boot(void);
void wake_debug_resume(void);
