/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __KERNEL_IPI_H_
#define __KERNEL_IPI_H_

#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>
#include "kernel_base.h"
#include "apic_ia32.h"

/*
    Private KERNEL IPI messages
*/
#define IPI_NOP         (APIC_EXCEPT_IPI_NOP - X86_CPU_EXCEPT_COUNT)
#define IPI_STOP        (APIC_EXCEPT_IPI_STOP - X86_CPU_EXCEPT_COUNT)
#define IPI_RESUME      (APIC_EXCEPT_IPI_RESUME - X86_CPU_EXCEPT_COUNT)
#define IPI_RESCHEDULE  (APIC_EXCEPT_IPI_RESCHEDULE - X86_CPU_EXCEPT_COUNT)
#define IPI_CALL_HOOK   (APIC_EXCEPT_IPI_CALL_HOOK - X86_CPU_EXCEPT_COUNT)
#define IPI_CAUSE       (APIC_EXCEPT_IPI_CAUSE - X86_CPU_EXCEPT_COUNT)

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
