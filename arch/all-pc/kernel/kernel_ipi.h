/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef __KERNEL_IPI_H_
#define __KERNEL_IPI_H_

#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>
#include "kernel_base.h"
#include "apic_ia32.h"

/*
    Private KERNEL IPI messages.

    Each message is the offset of its vector from APIC_EXCEPT_IPI_NOP:
    core_DoIPI() sends vector APIC_EXCEPT_IPI_NOP + message, and the
    handler is registered with exactly that offset as its data
    (apic_ia32.c). They used to be counted from X86_CPU_EXCEPT_COUNT,
    one below APIC_EXCEPT_IPI_NOP, so every message travelled on the
    next vector up and IPI_CAUSE on the APIC error vector.
*/
#define IPI_NOP         (APIC_EXCEPT_IPI_NOP - APIC_EXCEPT_IPI_NOP)
#define IPI_STOP        (APIC_EXCEPT_IPI_STOP - APIC_EXCEPT_IPI_NOP)
#define IPI_RESUME      (APIC_EXCEPT_IPI_RESUME - APIC_EXCEPT_IPI_NOP)
#define IPI_RESCHEDULE  (APIC_EXCEPT_IPI_RESCHEDULE - APIC_EXCEPT_IPI_NOP)
#define IPI_CALL_HOOK   (APIC_EXCEPT_IPI_CALL_HOOK - APIC_EXCEPT_IPI_NOP)
#define IPI_CAUSE       (APIC_EXCEPT_IPI_CAUSE - APIC_EXCEPT_IPI_NOP)

int core_IPIHandle(struct ExceptionContext *regs, void *data1, struct KernelBase *KernelBase);
void core_DoIPI(uint8_t ipi_number, void *cpu_mask, struct KernelBase *KernelBase);
int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, int nargs, IPTR *args, APTR _KB);

/* Drop the queued asynchronous calls of hookEntry whose second argument is
   matchArg, and wait out one that is running. Exec uses it when a task is
   removed, for the Signal() calls still on their way to it. */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg);

/* Ask the CPUs in the mask to reschedule; a pending request is not sent twice */
void core_RescheduleCPUs(void *cpu_mask, struct KernelBase *KernelBase);

/* struct IPIHook, the call queues and the request bits are shared by all ports */
#include <kernel_ipicall.h>

#endif /* __KERNEL_IPI_H_ */
