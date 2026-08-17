#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "vserr.h"
#include "char_fifo.h"

vserr_t char_fifo_init(struct char_fifo *cf, uint16_t capacity) {
    // Needs to be divisible by 8.
    if (capacity == 0 || capacity & 7) {
        return VSErr_InvParam;
    }

    cf->capacity = capacity;
    cf->read_index = 0;
    cf->write_index = 0;

    cf->items = (uint8_t *)AllocMem(capacity, MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
    if (!cf->items) {
        return VSErr_BufErr;
    }

    return VSErr_Success;
}

void char_fifo_deinit(struct char_fifo *cf) {
    FreeMem(cf->items, cf->capacity);
    cf->items = NULL;
    cf->capacity = 0;
    cf->read_index = 0;
    cf->write_index = 0;
}

void char_fifo_reset(struct char_fifo *cf) {
    cf->read_index = 0;
    cf->write_index = 0;
}

vserr_t char_fifo_enqueue(struct char_fifo *cf, uint8_t c) {
    const uint16_t write_index = cf->write_index;
    const uint16_t next_write_index = (write_index + 1) & (cf->capacity - 1);

    if (next_write_index == cf->read_index) {
        return VSErr_BadLength;
    }

    cf->items[write_index] = c;
    cf->write_index = next_write_index;

    return VSErr_Success;
}

vserr_t char_fifo_enqueue_n(struct char_fifo *cf, const uint8_t *buffer, uint16_t len) {
    const uint16_t available_capacity = cf->capacity - char_fifo_get_length(cf);
    if (len > available_capacity) {
        return VSErr_BadLength;
    }

    // TODO: Simplify this into 1-2 memcpys.

    const uint16_t capacity = cf->capacity;
    uint16_t write_index = cf->write_index;
    uint8_t *items = cf->items;

    while (len-- > 0) {
        items[write_index] = *buffer++;
        write_index = (write_index + 1) & (capacity - 1);
    }

    cf->write_index = write_index;

    return VSErr_Success;
}

vserr_t char_fifo_dequeue(struct char_fifo *cf, uint8_t *c) {
    if (cf->read_index == cf->write_index) {
        return VSErr_BadLength;
    }
    const uint16_t read_index = cf->read_index;
    *c = cf->items[read_index];
    cf->read_index = (read_index + 1) & (cf->capacity - 1);
    return VSErr_Success;
}

vserr_t char_fifo_dequeue_n(struct char_fifo *cf, uint8_t *buffer, uint16_t len) {
    const uint16_t available_length = char_fifo_get_length(cf);
    if (len > available_length) {
        return VSErr_BadLength;
    }

    // TODO: Simplify this into some memcpys.

    const uint16_t capacity = cf->capacity;
    uint16_t read_index = cf->write_index;
    const uint8_t *items = cf->items;

    while (len-- > 0) {
        *buffer++ = items[read_index];
        read_index = (read_index + 1) & (capacity - 1);
    }

    cf->read_index = read_index;

    return VSErr_Success;
}

uint16_t char_fifo_get_length(const struct char_fifo *cf) {
    const uint16_t read_index = cf->read_index;
    const uint16_t write_index = cf->write_index;
    return write_index < read_index ? (int32_t)(cf->capacity + write_index) - read_index : write_index - read_index;
}
