/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdbool.h>
#include <hardware/custom.h>
#include "util.h"
#include "buffy.h"

extern struct Custom custom;

bool buffy_open(void) {
    // Read once to empty the rx buffer.
    uint16_t serdatr = custom.serdatr;

    custom.serper = BUFFY_CMD_UNLOCK_REQUEST;

    do {
        wait_at_least_one_scanline();
        serdatr = custom.serdatr;
    } while (!(serdatr & SERDATR_RBF));
    
    const uint8_t challenge_code = serdatr & 0xFF;
    custom.serper = BUFFY_CMD_UNLOCK_RESPONSE_PREFIX | challenge_code;

    do {
        wait_at_least_one_scanline();
        serdatr = custom.serdatr;
    } while (!(serdatr & SERDATR_RBF));

    const uint8_t unlock_result = serdatr & 0xFF;
    if (unlock_result != 0x00) {
        return false;
    }

    buffy_reset();

    // Set default host state.
    custom.serper = BUFFY_HOST_STATE_PREFIX | BUFFY_HOST_STATE_CTS_ACTIVE | BUFFY_HOST_STATE_RTS_ACTIVE;
    wait_at_least_one_scanline();
    return true;
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
