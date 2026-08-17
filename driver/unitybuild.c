/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */

// The GCC toolchain doesn't support LTO, so this is a way around that.
// Provides a much smaller binary.
#include "device.c"
#include "buffy.c"
#include "util.c"
#include "char_fifo.c"