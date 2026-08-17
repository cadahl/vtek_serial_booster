/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CHAR_FIFO_H
#define CHAR_FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include "vserr.h"

struct char_fifo {
    uint16_t capacity;
    uint16_t read_index;
    uint16_t write_index;
    uint8_t *items;
};

/*
    Initialize a FIFO, allocating memory according to the parameters.
*/
VSFUNC vserr_t char_fifo_init(struct char_fifo *cf, uint16_t capacity);

/*
    Deinitialize a FIFO, freeing allocated memory.
*/
VSFUNC void char_fifo_deinit(struct char_fifo *cf);

/*
    Get the number of characters currently in a FIFO.
*/
VSFUNC uint16_t char_fifo_get_length(const struct char_fifo *cf);

/*
    Get the number of characters that will fit in the remaining space in a FIFO.
*/
VSFUNC uint16_t char_fifo_get_remaining_capacity(const struct char_fifo *cf);

/*
    Reset the read/write pointers in a FIFO, effectively clearing it.
*/
VSFUNC void char_fifo_reset(struct char_fifo *cf);

/*
    Enqueue a character in a FIFO.

    Returns VSErr_BadLength if the FIFO is full.
*/
VSFUNC vserr_t char_fifo_enqueue(struct char_fifo *cf, uint8_t c);

/*
    Enqueue N characters in a FIFO.

    Returns VSErr_BadLength if the FIFO does not contain the number of characters specified.
*/
VSFUNC vserr_t char_fifo_enqueue_n(struct char_fifo *cf, const uint8_t *buffer, uint16_t len);

/*
    Dequeue a character from a FIFO.

    Returns VSErr_BadLength if the FIFO is empty.
*/
VSFUNC vserr_t char_fifo_dequeue(struct char_fifo *cf, uint8_t *c);

/*
    Dequeue N characters from a FIFO.

    Returns VSErr_BadLength if the FIFO does not contain the number of characters specified.
*/
VSFUNC vserr_t char_fifo_dequeue_n(struct char_fifo *cf, uint8_t *buffer, uint16_t len);

#endif
