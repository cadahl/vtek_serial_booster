/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdbool.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include "util.h"
#include "buffy.h"

extern struct Custom custom;

static vserr_t transmit_char(uint8_t c);
static vserr_t receive_char(uint8_t *c);

VSFUNC vserr_t buffy_open(void) {
    // Read once to empty the rx buffer.
    uint16_t serdatr = custom.serdatr;
    (void)serdatr;

    custom.serper = BUFFY_CMD_UNLOCK_REQUEST;
    
    uint8_t challenge_high = 0;
    uint8_t challenge_low = 0;
    vserr_t err = receive_char(&challenge_high);
    if (err) {
        return VSErr_HardwareNotPresent;
    }
    err = receive_char(&challenge_low);
    if (err) {
        return VSErr_HardwareNotPresent;
    }

    uint16_t challenge = (challenge_high << 8) | challenge_low;
    uint16_t response = challenge * 0x8D27;

    err = transmit_char(response >> 8);
    if (err) {
        return VSErr_HardwareNotPresent;
    }
    err = transmit_char(response & 0xFF);
    if (err) {
        return VSErr_HardwareNotPresent;
    }

    uint8_t unlock_result = 0;
    err = receive_char(&unlock_result);
    if (err) {
        return VSErr_HardwareNotPresent;
    }

    if (unlock_result != 0x00) {
        return VSErr_HardwareCommandFailed;
    }

    buffy_reset();

    return VSErr_Success;
}

VSFUNC void buffy_reset(void) {
    custom.serper = BUFFY_CMD_RESET;
    wait_at_least_one_scanline();
}

VSFUNC void buffy_close(void) {
    custom.serper = BUFFY_CMD_LOCK;
    wait_at_least_one_scanline();

    custom.serper = BUFFY_CMD_RESET;
    wait_at_least_one_scanline();
}

VSFUNC vserr_t buffy_set_config_block(const struct buffy_config_block *cfg) {
    // Read once to empty the rx buffer.
    uint16_t serdatr = custom.serdatr;
    (void)serdatr;

    custom.serper = BUFFY_CMD_WRITE_CONFIG_BLOCK;

    vserr_t err = VSErr_Success;
    const uint8_t *data = (const uint8_t *)cfg;

    for (uint16_t i = 0; i < sizeof(struct buffy_config_block); ++i) {
        err = transmit_char(data[i]);
        if (err) {
            return err;
        }
    }    

    return err;
}

VSFUNC vserr_t buffy_get_config_block(struct buffy_config_block *cfg) {
    // Read once to empty the rx buffer.
    uint16_t serdatr = custom.serdatr;
    (void)serdatr;

    custom.serper = BUFFY_CMD_READ_CONFIG_BLOCK;

    vserr_t err = VSErr_Success;
    uint8_t *data = (uint8_t *)cfg;

    for (uint16_t i = 0; i < sizeof(struct buffy_config_block); ++i) {
        err = receive_char(&data[i]);
        if (err) {
            return err;
        }
    }

    return err;
}

static vserr_t transmit_char(uint8_t c) {
    uint16_t attempts_remaining = 10;
    uint16_t intreqr = custom.intreqr;
    while (--attempts_remaining > 0 && !(intreqr & INTF_TBE)) {
        wait_at_least_one_scanline();
        intreqr = custom.intreqr;
    }

    if (attempts_remaining == 0) {
        return VSErr_HardwareCommandFailed;
    }

    custom.serdat = 0x100 | c;
 
    return VSErr_Success;
}

static vserr_t receive_char(uint8_t *c) {
    uint16_t attempts_remaining = 10;
    uint16_t serdatr = custom.serdatr;
    while (--attempts_remaining > 0 && !(serdatr & SERDATR_RBF)) {
        wait_at_least_one_scanline();
        serdatr = custom.serdatr;
    }

    if (attempts_remaining == 0) {
        return VSErr_HardwareCommandFailed;
    }
 
    return VSErr_Success;
}
