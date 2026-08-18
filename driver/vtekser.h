/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef VTEKSER_H
#define VTEKSER_H

#include "buffy_defs.h"

/* 
    Set Configuration

    io_Data     the buffy_config_block to set
    io_Length   must be set to sizeof(struct buffy_config_block) 
*/
#define   VTSDCMD_SET_CFG		(CMD_NONSTD + 16)	

/* 
    Get Configuration

    io_Data     enough space for the returned buffy_config_block
    io_Length   must be set to sizeof(struct buffy_config_block) 
*/
#define   VTSDCMD_GET_CFG		(CMD_NONSTD + 17)	

#endif
