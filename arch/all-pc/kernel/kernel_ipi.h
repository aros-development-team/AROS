/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
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

#define IPI_CALL_HOOK_MAX_ARGS  5

/*
    IPI Call hook
*/
struct IPIHook
{
    struct Hook     ih_Hook;
    IPTR            ih_Args[IPI_CALL_HOOK_MAX_ARGS];
    uint32_t *      ih_CPUDone;
    uint32_t *      ih_CPURequested;
    int             ih_Async;
    spinlock_t      ih_Lock;
    spinlock_t      ih_SyncLock;
};

#endif /* __KERNEL_IPI_H_ */
