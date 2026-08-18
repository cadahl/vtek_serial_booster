/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BUFFY_DEFS_H
#define BUFFY_DEFS_H

#include <stddef.h>

/*
    Buffy commands are encoded in the bottom seven bits of a special SERPER write.
    Pattern: L1111111 1xxxxxxx
    where L is the current long mode bit. 

    Unlock special functionality:
    - Amiga writes 0x7F80 to SERPER
    - Buffy sends a random byte e.g. 0xAA
    - Amiga writes 0x7FAA to SERPER 
    - Buffy sends 0x00 on success, or error code.
    - This unlocks the other commands.

    Lock special functionality again:
    - Amiga writes 0x7F81 to SERPER.
    - Buffy sends 0x00 on success, or error code.
*/

enum buffy_command {
    BUFFY_CMD_UNLOCK_REQUEST       = 0x7F80,
    BUFFY_CMD_LOCK                 = 0x7F81,

    BUFFY_CMD_SET_BAUD_RATE        = 0x7F82,    // followed by 4 TX characters (32-bit big-endian baud rate)

    BUFFY_CMD_WRITE_CONFIG_BLOCK   = 0x7FFB,
    BUFFY_CMD_READ_CONFIG_BLOCK    = 0x7FFC,

    BUFFY_CMD_TEST_MODE            = 0x7FFD,
    BUFFY_CMD_RESET                = 0x7FFE,
};

#define BUFFY_CMD_UNLOCK_RESPONSE_PREFIX    0x7F00

/*
    Host state is encoded in the bottom seven bits of a special SERPER write.
    Pattern: L1111111 0xxxxxxx    
    where L is the current long mode bit. 
*/
#define BUFFY_HOST_STATE_PREFIX             0x7F00
#define BUFFY_HOST_STATE_MASK               0x007F

#define BUFFY_HOST_STATE_RTS_ACTIVE         0x01
#define BUFFY_HOST_STATE_CTS_ACTIVE         0x02
#define BUFFY_HOST_STATE_HBLANK_INT2_WANTED 0x04
#define BUFFY_HOST_STATE_XONXOFF_ENABLED    0x08

/*
    Config block.
*/
#define BUFFY_CONFIG_BLOCK_FLAGS_RX_FIFO                (1U << 0)   // RX FIFO enabled
#define BUFFY_CONFIG_BLOCK_FLAGS_TX_FIFO                (1U << 1)   // TX FIFO enabled
#define BUFFY_CONFIG_BLOCK_FLAGS_OVERRIDE_REGION        (1U << 2)   // Override region?
#define BUFFY_CONFIG_BLOCK_FLAGS_OVERRIDE_REGION_IS_PAL (1U << 3)   // Override region: force PAL (if not set: force NTSC)
#define BUFFY_CONFIG_BLOCK_FLAGS_USE_CTS_RTS            (1U << 4)   // Use external CTS/RTS signals from CIA chip

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
