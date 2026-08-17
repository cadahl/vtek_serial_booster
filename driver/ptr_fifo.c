/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "vserr.h"
#include "ptr_fifo.h"
#include "util.h"

VSFUNC vserr_t ptr_fifo_init(struct ptr_fifo *pf, uint16_t capacity) {
    if (capacity == 0) {
        return VSErr_InvParam;
    }

    pf->capacity = capacity;
    pf->read_index = 0;
    pf->write_index = 0;

    pf->items = (void **)AllocMem(capacity * sizeof(void *), MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
    if (!pf->items) {
        return VSErr_BufErr;
    }

    return VSErr_Success;
}

VSFUNC void ptr_fifo_deinit(struct ptr_fifo *pf) {
    FreeMem(pf->items, pf->capacity * sizeof(void *));
    pf->items = NULL;
    pf->capacity = 0;
    pf->read_index = 0;
    pf->write_index = 0;
}

VSFUNC void ptr_fifo_reset(struct ptr_fifo *pf) {
    pf->read_index = 0;
    pf->write_index = 0;
}

VSFUNC vserr_t ptr_fifo_enqueue(struct ptr_fifo *pf, void *ptr) {
    const uint16_t write_index = pf->write_index;
    const uint16_t next_write_index = (write_index + 1) & (pf->capacity - 1);

    if (next_write_index == pf->read_index) {
        return VSErr_BadLength;
    }

    pf->items[write_index] = ptr;
    pf->write_index = next_write_index;

    return VSErr_Success;
}

VSFUNC vserr_t ptr_fifo_dequeue(struct ptr_fifo *pf, void **ptr) {
    if (pf->read_index == pf->write_index) {
        return VSErr_BadLength;
    }
    const uint16_t read_index = pf->read_index;
    if (ptr) {
        *ptr = pf->items[read_index];
    }
    pf->read_index = (read_index + 1) & (pf->capacity - 1);
    return VSErr_Success;
}

VSFUNC vserr_t ptr_fifo_peek(struct ptr_fifo *pf, void **ptr) {
    if (pf->read_index == pf->write_index) {
        return VSErr_BadLength;
    }
    const uint16_t read_index = pf->read_index;
    *ptr = pf->items[read_index];
    return VSErr_Success;
}

VSFUNC bool ptr_fifo_is_empty(const struct ptr_fifo *pf) {
    return pf->read_index == pf->write_index;
}

VSFUNC uint16_t ptr_fifo_get_length(const struct ptr_fifo *pf) {
    const uint16_t read_index = pf->read_index;
    const uint16_t write_index = pf->write_index;
    return write_index < read_index ? (int32_t)(pf->capacity + write_index) - read_index : write_index - read_index;
}
