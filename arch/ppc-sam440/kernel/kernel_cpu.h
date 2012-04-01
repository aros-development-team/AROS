/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef KERNEL_CPU_H
#define KERNEL_CPU_H

#include <asm/amcc440.h>
#include "kernel_syscall.h"

#define EXCEPTIONS_COUNT        16

typedef uint32_t cpumode_t;

static inline uint32_t goSuper() {
        register uint32_t oldmsr asm("r3");
        asm volatile("li %0,%1; sc":"=r"(oldmsr):"i"(SC_SUPERSTATE):"memory");
        return oldmsr;
}

static inline void goUser() {
    wrmsr(rdmsr() | (MSR_PR));
}

static inline void goBack(uint32_t oldmsr)
{
    wrmsr(oldmsr);
}

#define krnSysCall(n) \
    asm volatile("li %%r3,%0; sc"::"i"(n):"memory","r3");

typedef context_t regs_t;

#endif /* KERNEL_CPU_H */
