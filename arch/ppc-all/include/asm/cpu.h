#ifndef ASM_PPC_CPU_H
#define ASM_PPC_CPU_H

#include <inttypes.h>

static inline uint32_t rdmsr()
{
    uint32_t msr; asm volatile("mfmsr %0":"=r"(msr)); return msr;
}

static inline void wrmsr(uint32_t msr)
{
    asm volatile("mtmsr %0"::"r"(msr));
}

#define rdspr(reg) \
    ({ unsigned long val; asm volatile("mfspr %0,%1":"=r"(val):"i"(reg)); val; })

#define wrspr(reg, val) \
    do { asm volatile("mtspr %0,%1"::"i"(reg),"r"(val)); } while(0)

#endif
