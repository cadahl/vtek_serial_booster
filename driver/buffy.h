/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BUFFY_H
#define BUFFY_H

#include <stdbool.h>
#include "vserr.h"
#include "buffy_defs.h"

VSFUNC vserr_t buffy_open(void);
VSFUNC void buffy_reset(void);
VSFUNC void buffy_close(void);
VSFUNC vserr_t buffy_set_config_block(const struct buffy_config_block *cfg);
VSFUNC vserr_t buffy_get_config_block(struct buffy_config_block *cfg);

#endif
