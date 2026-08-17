/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BUFFY_H
#define BUFFY_H

#include <stdbool.h>
#include "vserr.h"
#include "util.h"

VSFUNC vserr_t buffy_open(void);
VSFUNC void buffy_reset(void);
VSFUNC void buffy_close(void);

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

#endif
