/* SPDX-FileCopyrightText: Copyright (c) 2026 Carl Ådahl / VTek */
/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define STR(s) #s      /* Turn s into a string literal without expanding macro definitions (however, \
                          if invoked from a macro, macro arguments are expanded). */
#define XSTR(s) STR(s) /* Turn s into a string literal after macro-expanding it. */

#define NOP() __asm__ __volatile__ ("nop")

void wait_us(uint16_t microseconds);
void wait_at_least_one_scanline(void);

#define SERDATR_OVF 0x8000  
#define SERDATR_RBF 0x4000  

#if DEBUG
/* KPrintF is provided either by libdebug.a (Bebbo's toolchain, linked via
   -ldebug -mcrt=clib2) or by debug.c in this repo (Bartman's elf-toolchain,
   which has no libdebug). Either way we just declare the prototype here. */
extern void KPrintF(CONST_STRPTR fmt, ...);
#endif

#endif
