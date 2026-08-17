/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#include <hardware/custom.h>
#include "util.h"

extern struct Custom custom;

void wait_at_least_one_scanline(void) {
    uint16_t first = custom.vhposr & 0xFF00;
    uint16_t second = first;
    do {
        // Leave some room for DMA - 256 clock cycles.
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        second = custom.vhposr & 0xFF00;
    } while(second == first);
    uint16_t third = second;
    do {
        // Leave some room for DMA - 256 clock cycles.
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
        third = custom.vhposr & 0xFF00;
    } while(third == second);
}

void wait_us(uint16_t us) {
    uint16_t lines_to_wait = (us + 63) / 64;     
    uint16_t last_line = custom.vhposr & 0xFF00;
    uint16_t lines_counted = 0;

    while (lines_counted < lines_to_wait) {
        uint16_t current_line = custom.vhposr & 0xFF00;
        if (current_line != last_line) {
            lines_counted++;
            last_line = current_line;
        }
    }
}
