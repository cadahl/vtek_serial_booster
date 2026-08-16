#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define NOP() __asm__ __volatile__ ("nop")

void wait_us(uint16_t microseconds);
void wait_at_least_one_scanline(void);

#define SERDATR_OVF 0x8000  
#define SERDATR_RBF 0x4000  

#endif
