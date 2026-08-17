/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdbool.h>
#include <hardware/custom.h>
#include "util.h"
#include "buffy.h"

extern struct Custom custom;

vserr_t buffy_open(void) {
    const int max_attempts = 16;

    // Read once to empty the rx buffer.
    uint16_t serdatr = custom.serdatr;

    custom.serper = BUFFY_CMD_UNLOCK_REQUEST;

    int attempts = 1;
    serdatr = custom.serdatr;
    while (attempts < max_attempts && !(serdatr & SERDATR_RBF)) {
        wait_at_least_one_scanline();
        serdatr = custom.serdatr;
        ++attempts;
    }
    
    const uint8_t challenge_code = serdatr & 0xFF;
    custom.serper = BUFFY_CMD_UNLOCK_RESPONSE_PREFIX | challenge_code;

    attempts = 1;
    serdatr = custom.serdatr;
    while (attempts < max_attempts && !(serdatr & SERDATR_RBF)) {
        wait_at_least_one_scanline();
        serdatr = custom.serdatr;
        ++attempts;
    }

    const uint8_t unlock_result = serdatr & 0xFF;
    if (unlock_result != 0x00) {
        return VSErr_HardwareNotPresent;
    }

    buffy_reset();

    // Set default host state.
    custom.serper = BUFFY_HOST_STATE_PREFIX | BUFFY_HOST_STATE_CTS_ACTIVE | BUFFY_HOST_STATE_RTS_ACTIVE;
    wait_at_least_one_scanline();

    return VSErr_Success;
}

void buffy_reset(void) {
    custom.serper = BUFFY_CMD_RESET;
    wait_at_least_one_scanline();
}

void buffy_close(void) {
    custom.serper = BUFFY_HOST_STATE_PREFIX | BUFFY_HOST_STATE_CTS_ACTIVE | BUFFY_HOST_STATE_RTS_ACTIVE;
    wait_at_least_one_scanline();
    custom.serper = BUFFY_CMD_LOCK;
    wait_at_least_one_scanline();
}
