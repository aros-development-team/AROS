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

#define goSuper() 0
#define goUser() do { } while (0)
#define goBack(val) do { } while (0)

#define krnSysCall(n) \
    asm volatile("li %%r3,%0; sc"::"i"(n):"memory","r3");

/* Architecture specific syscalls */
#define SC_IRQ_ENABLE   0x201
#define SC_IRQ_DISABLE  0x202

#define krnSysCall(n) \
    asm volatile("li %%r3,%0; sc"::"i"(n):"memory","r3");

#define krnSysCall1(n, arg) \
    asm volatile("lwz %%r4,%1; li %%r3,%0; sc"::"i"(n),"m"(arg):"memory","r3","r4");

typedef context_t regs_t;

#endif /* KERNEL_CPU_H */
