/*
 * Copyright (C) 2012-2026, The AROS Development Team
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

#define goSuper() 0
#define goUser() do { } while (0)
#define goBack(val) do { } while (0)

/* Architecture specific syscalls */
#define SC_IRQ_ENABLE   0x201
#define SC_IRQ_DISABLE  0x202

#define krnSysCall(n) \
    asm volatile("li %%r3,%0; sc;\n\t"::"i"(n):"memory","r3");

/* widen potentially smaller typed arg to register sized uint32_t to avoid
 * usage of uninitialized data
 */
#define krnSysCall1(n, arg) \
    do { \
        uint32_t _arg = (uint32_t)(arg); \
        asm volatile("mr %%r4,%1; li %%r3,%0\n\t; sc"::"i"(n),"r"(_arg):"memory","r3","r4"); \
    } while (0)

typedef context_t regs_t;

#endif /* KERNEL_CPU_H */
