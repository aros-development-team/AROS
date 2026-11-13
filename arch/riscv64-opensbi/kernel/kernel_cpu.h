/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: opensbi-riscv64 kernel CPU definitions.
*/

#ifndef KERNEL_CPU_RISCV64_H_
#define KERNEL_CPU_RISCV64_H_

#include <inttypes.h>

#define EXCEPTIONS_COUNT	16

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

/*
 * Context switch protocol for the extension state (see asm/cpu.h for the
 * sstatus FS/VS fields):
 *
 *  - FPU (FS): the kernel runs with FS enabled (LP64D code may use FP
 *    registers anywhere). On dispatch the scheduler sets FS=CLEAN; on
 *    switch-out it saves f0-f31/fcsr into ctx->fpuContext (setting
 *    ECF_FPU) only when FS reads back DIRTY.
 *
 *  - Vector (VS): presence is detected at runtime (__riscv_vlenb != 0,
 *    see cpu_init.c) and ctx->vecContext is only laid out then. Tasks
 *    start with VS=OFF; the first vector instruction traps, the handler
 *    enables VS and marks the task vector-using. From then on the
 *    scheduler saves/restores v0-v31 and vstart/vtype/vl/vcsr through
 *    ctx->vecContext (setting ECF_VECTOR) when VS reads back DIRTY.
 */

#define ADDTIME(dest, src)			        \
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						                \
        (dest)->tv_secs++;			        \
        (dest)->tv_micro -= 1000000;		\
    }

#define goSuper() 0
#define goUser()

/*
 * Kernel syscall entry. ecall from S-mode always traps to M-mode - it
 * is the SBI call mechanism and OpenSBI does not (and must not)
 * delegate it. Breakpoints ARE delegated to S-mode, so the scheduler
 * syscalls use ebreak with the function code in a7; the trap handler
 * distinguishes them from real breakpoints by a7.
 */
#undef krnSysCall
#define krnSysCall(n) \
    asm volatile ( \
    "\taddi a7, zero, %[swi_no]\n" \
    "\tebreak\n" \
    : : [swi_no] "I" (n) : "a7", "memory");

#endif /* KERNEL_CPU_RISCV64_H_ */
