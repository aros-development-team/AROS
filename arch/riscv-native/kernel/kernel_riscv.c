/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Native RISC-V kernel state shared by every platform - the
          firmware hook table and the logical CPU numbering.
*/

#include <aros/config.h>
#include <exec/types.h>
#include <stddef.h>

#include "kernel_riscv.h"

/* Filled in by the platform (riscv64-opensbi: kernel_startup.c) */
struct RISCV_Implementation __riscv_arosintern;

/* The boot hart is CPU 0 and online from the first instruction */
uint32_t __riscv_cpu_count = 1;
ULONG    __riscv_cpu_online[RISCV_CPUMASK_WORDS] = { 1 };

#if defined(__AROSEXEC_SMP__)
/*
 * The boot hart's TLS is static so that tp is valid before anything
 * can be allocated - startup.S loads it before the first C call, and
 * traps.S counts nesting through it from then on.
 */
tls_t    __riscv_boot_tls;
tls_t    *__riscv_tls[RISCV_MAXCPUS] = { &__riscv_boot_tls };

_Static_assert(offsetof(tls_t, ThisTask) == TLS_THISTASK_OFFSET,
               "stackswap.S relies on TLS_THISTASK_OFFSET");
_Static_assert(offsetof(tls_t, TrapDepth) == TLS_TRAPDEPTH_OFFSET,
               "traps.S relies on TLS_TRAPDEPTH_OFFSET");
_Static_assert(offsetof(tls_t, HartID) == TLS_HARTID_OFFSET,
               "startup.S relies on TLS_HARTID_OFFSET");
_Static_assert(offsetof(tls_t, StackTop) == TLS_STACKTOP_OFFSET,
               "startup.S relies on TLS_STACKTOP_OFFSET");
#endif

#if defined(__AROSEXEC_SMP__)
#include "kernel_ipi.h"

void krnRVResetCPUs(void)
{
    int i;

    /* .data survives a warm restart; the harts it describes do not */
    __riscv_cpu_count = 1;
    for (i = 0; i < RISCV_CPUMASK_WORDS; i++)
        __riscv_cpu_online[i] = 0;
    __riscv_cpu_online[0] = 1;
    for (i = 1; i < RISCV_MAXCPUS; i++)
        __riscv_tls[i] = NULL;
    __riscv_tls[0] = &__riscv_boot_tls;
}

BOOL krnRVStopOtherCPUs(void)
{
    ULONG mask[RISCV_CPUMASK_WORDS];
    uint32_t self = krnRVCPUNumber();
    UQUAD start, now, limit;

    if (__riscv_cpu_count < 2)
        return TRUE;

    krnRVResolveMask((void *)(IPTR)-2, self, mask);   /* all but self */
    core_SendIPI(IPI_REBOOT, mask);

    /* Each one drops out of the online mask just before it stops */
    limit = __riscv_arosintern.RVI_TimebaseFreq ?
            __riscv_arosintern.RVI_TimebaseFreq : 10000000;
    asm volatile("rdtime %0" : "=r"(start));
    while (__atomic_load_n(&__riscv_cpu_count, __ATOMIC_ACQUIRE) > 1)
    {
        asm volatile("rdtime %0" : "=r"(now));
        if ((now - start) > limit)
            return FALSE;
        krnRVPause();
    }
    return TRUE;
}

/* The calling hart leaves the online mask (it is about to stop) */
void krnRVCPUOffline(uint32_t cpu)
{
    ULONG bit = 1UL << (cpu % 32);

    if (cpu >= RISCV_MAXCPUS)
        return;

    if (__atomic_fetch_and(&__riscv_cpu_online[cpu / 32], ~bit, __ATOMIC_SEQ_CST) & bit)
        __atomic_fetch_sub(&__riscv_cpu_count, 1, __ATOMIC_SEQ_CST);
}
#endif

volatile UQUAD __riscv_cpu_idletime[RISCV_MAXCPUS];

/*
 * The idle tasks' wait. Interrupts are masked around the wfi so that
 * the span measured is the sleep alone: wfi still wakes on a pending
 * interrupt, which is then taken - and may run other tasks - only once
 * the time has been counted.
 */
void krnRVIdleWait(void)
{
    uint32_t cpu = krnRVCPUNumber();
    unsigned int prev = krnRVIntrDisable();
    UQUAD from, to;

    asm volatile("rdtime %0" : "=r"(from));
    asm volatile("wfi" ::: "memory");
    asm volatile("rdtime %0" : "=r"(to));

    if (cpu < RISCV_MAXCPUS)
        __riscv_cpu_idletime[cpu] += to - from;

    krnRVIntrRestore(prev);
}

void krnRVCPUOnline(uint32_t cpu)
{
    ULONG bit = 1UL << (cpu % 32);

    if (cpu >= RISCV_MAXCPUS)
        return;

    if (!(__atomic_fetch_or(&__riscv_cpu_online[cpu / 32], bit, __ATOMIC_SEQ_CST) & bit))
        __atomic_fetch_add(&__riscv_cpu_count, 1, __ATOMIC_SEQ_CST);
}
