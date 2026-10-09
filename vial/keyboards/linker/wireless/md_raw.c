// Copyright 2024 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#if RAW_ENABLE

#    include "quantum.h"
#    include "wireless.h"
#    include "usb_endpoints.h"
#    include "usb_main.h"

// Wireless host driver's send_raw_hid; over USB the ChibiOS driver sends.
void wireless_send_raw_hid(uint8_t *data, uint8_t length) {
    if (length != RAW_EPSIZE) {
        return;
    }
    md_send_raw(data, length);
}

void md_receive_raw_cb(uint8_t *data, uint8_t length) {
    void raw_hid_receive(uint8_t * data, uint8_t length);

    raw_hid_receive(data, length);
}

#endif
