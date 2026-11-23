#ifndef KERNEL_IPIPORT_H_
#define KERNEL_IPIPORT_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: What this port supplies for rom/kernel/kernel_ipicall.c.
          See rom/kernel/kernel_ipicall.h for what each one must do.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#include "kernel_cpu.h"

#define KRN_IPICALL
#define KRN_IPICALL_MAXCPUS     4
/* The queues are set up before exec's allocator can be used */
#define KRN_IPICALL_STATIC

/* IPI_CALL_HOOK (kernel_ipi.h): the IPI_* messages are the request bits here */
#define KRN_IPIF_CALL           0x040

void krnIPIPortRaise(ULONG cpu, ULONG bits);
BOOL krnIPIPortCanServe(void);
void krnIPIPortServe(void);

#define krnIPIPortThisCPU()             ((ULONG)GetCPUNumber())
#define krnIPIPortPause()               asm volatile("yield" ::: "memory")

/* IRQ and FIQ: the IPI is one or the other, depending on the board */
static inline ULONG krnIPIPortIntrDisable(void)
{
    unsigned long daif;

    asm volatile("mrs %0, daif\n\tmsr daifset, #3" : "=r"(daif) :: "memory");

    return (ULONG)daif;
}

static inline void krnIPIPortIntrRestore(ULONG flags)
{
    unsigned long daif = flags;

    asm volatile("msr daif, %0" :: "r"(daif) : "memory");
}

#endif /* __AROSEXEC_SMP__ */

#endif /* KERNEL_IPIPORT_H_ */
