/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CONFIG_H
#define CONFIG_H

// Amiga-side FIFO lengths. Note: this is in characters, and space for 9 bits per char is allocated.
#define RX_FIFO_LEN 8192
#define TX_FIFO_LEN 8192

// Maximum number of queued IO read requests.
#define RRQ_LEN 8

#endif
