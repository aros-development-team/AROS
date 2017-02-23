/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id:$
*/

#ifndef __KERNEL_IPI_H_
#define __KERNEL_IPI_H_

#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>
#include "kernel_base.h"

/*
    Private KERNEL IPI messages
*/
#define IPI_NOP         0
#define IPI_STOP        1
#define IPI_RESUME      2
#define IPI_RESCHEDULE  3
#define IPI_CALL_HOOK   4
#define IPI_CAUSE       5

void core_IPIHandle(struct ExceptionContext *regs, unsigned long ipi_number, struct KernelBase *KernelBase);
void core_DoIPI(uint8_t ipi_number, void *cpu_mask, struct KernelBase *KernelBase);
void core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, struct KernelBase *KernelBase);

/*
    IPI Call hook
*/
struct IPIHook
{
    struct Hook     ih_Hook;
    uint32_t *      ih_CPUDone;
    uint32_t *      ih_CPURequested;
    int             ih_Async;
    spinlock_t      ih_Lock;
    spinlock_t      ih_SyncLock;
};

#endif /* __KERNEL_IPI_H_ */
