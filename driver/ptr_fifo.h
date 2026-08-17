/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef PTR_FIFO_H
#define PTR_FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include "vserr.h"
#include "util.h"

struct ptr_fifo {
    uint16_t capacity;
    uint16_t read_index;
    uint16_t write_index;
    void **items;
};

/*
    Initialize a FIFO, allocating memory according to the parameters.
*/
VSFUNC vserr_t ptr_fifo_init(struct ptr_fifo *pf, uint16_t capacity);

/*
    Deinitialize a FIFO, freeing allocated memory.
*/
VSFUNC void ptr_fifo_deinit(struct ptr_fifo *pf);

/*
    Return true if a FIFO is empty.
*/
VSFUNC bool ptr_fifo_is_empty(const struct ptr_fifo *pf);

/*
    Get the number of pointers currently in a FIFO.
*/
VSFUNC uint16_t ptr_fifo_get_length(const struct ptr_fifo *pf);

/*
    Reset the read/write pointers in a FIFO, effectively clearing it.
*/
VSFUNC void ptr_fifo_reset(struct ptr_fifo *pf);

/*
    Enqueue a pointer in a FIFO.

    Returns VSErr_BadLength if the FIFO is full.
*/
VSFUNC vserr_t ptr_fifo_enqueue(struct ptr_fifo *pf, void *ptr);

/*
    Dequeue a pointer from a FIFO.

    Returns VSErr_BadLength if the FIFO is empty.
*/
VSFUNC vserr_t ptr_fifo_dequeue(struct ptr_fifo *pf, void **ptr);

/*
    Peek at the first element of a FIFO.

    Returns VSErr_BadLength if the FIFO is empty.
*/
VSFUNC vserr_t ptr_fifo_peek(struct ptr_fifo *pf, void **ptr);

#endif
