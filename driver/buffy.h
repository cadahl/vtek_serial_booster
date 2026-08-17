/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BUFFY_H
#define BUFFY_H

#include <stdbool.h>
#include "vserr.h"
#include "util.h"
#include "buffy_defs.h"

VSFUNC vserr_t buffy_open(void);
VSFUNC void buffy_reset(void);
VSFUNC void buffy_close(void);

#endif
