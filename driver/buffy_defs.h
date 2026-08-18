/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BUFFY_DEFS_H
#define BUFFY_DEFS_H

#include <stddef.h>

enum buffy_command {
    BUFFY_CMD_UNLOCK_REQUEST        = 0x7F20,
    BUFFY_CMD_LOCK                  = 0x7F21,

    BUFFY_CMD_SET_BAUD_RATE         = 0x7F22,    // followed by 4 TX characters (32-bit big-endian baud rate)

    BUFFY_CMD_WRITE_CONFIG_BLOCK    = 0x7F23,
    BUFFY_CMD_READ_CONFIG_BLOCK     = 0x7F24,

    BUFFY_CMD_TEST_MODE             = 0x7F25,
    BUFFY_CMD_RESET                 = 0x7F26,

    BUFFY_CMD_SET_HOST_STATE_PREFIX = 0x7F80,
};

#define BUFFY_SERPER_START                  0x7F20  // values below this are useful baud rates of 110 and above.

#define BUFFY_SET_HOST_STATE_PARAM_MASK     0x007F

#define BUFFY_HOST_STATE_DSR                 0x0001
#define BUFFY_HOST_STATE_CTS                 0x0002
#define BUFFY_HOST_STATE_CD                  0x0004
#define BUFFY_HOST_STATE_RTS                 0x0008
#define BUFFY_HOST_STATE_DTR                 0x0010
#define BUFFY_HOST_STATE_HBLANK_INT2_WANTED  0x0020
#define BUFFY_HOST_STATE_XONXOFF_ENABLED     0x0040

/*
    Config block.
*/
#define BUFFY_CONFIG_BLOCK_FLAGS_RX_FIFO                0x01   // RX FIFO enabled
#define BUFFY_CONFIG_BLOCK_FLAGS_TX_FIFO                0x02   // TX FIFO enabled
#define BUFFY_CONFIG_BLOCK_FLAGS_OVERRIDE_REGION        0x04   // Override region?
#define BUFFY_CONFIG_BLOCK_FLAGS_OVERRIDE_REGION_IS_PAL 0x08   // Override region: force PAL (if not set: force NTSC)
#define BUFFY_CONFIG_BLOCK_FLAGS_USE_HARDWARE_CTS_RTS   0x10   // Use external hardware CTS/RTS signals

struct buffy_config_block {
    uint8_t crc16_lo;
    uint8_t crc16_hi;
    uint8_t seq;
    uint8_t version;

    uint8_t flags;
    uint8_t oschf_tune;
    uint8_t dummy2;
    uint8_t dummy3;

    uint8_t dummy4;
    uint8_t dummy5;
    uint8_t dummy6;
    uint8_t dummy7;

    uint8_t dummy8;
    uint8_t dummy9;
    uint8_t dummy10;
    uint8_t dummy11;
} __attribute__((packed));

static_assert(offsetof(struct buffy_config_block, crc16_lo) == 0, "crc16_lo in struct buffy_config_block must be at offset 0");
static_assert(offsetof(struct buffy_config_block, crc16_hi) == 1, "crc16_hi in struct buffy_config_block must be at offset 1");
static_assert(sizeof(struct buffy_config_block) == 16, "struct buffy_config_block must be 16 bytes");

#endif
