#ifndef ASM_I386_CPU_H
#define ASM_I386_CPU_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: cpu.h 38295 2011-04-21 11:01:07Z sonic $

    Desc: assembler-level specific definitions for x86-64 CPU
    Lang: english
*/

#include <inttypes.h>

/* This file is very very incomplete :) */

#define HALT asm volatile("hlt")

static inline void __attribute__((always_inline)) rdmsr(uint32_t msr_no, uint32_t *ret_lo, uint32_t *ret_hi)
{
    uint32_t ret1, ret2;

    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    *ret_lo=ret1;
    *ret_hi=ret2;
}

#endif
