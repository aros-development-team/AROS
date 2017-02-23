/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id:$
*/

#ifndef __KERNEL_IPI_H_
#define __KERNEL_IPI_H_

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

/*
    IPI Call hook
*/
struct IPIHook
{
    struct Hook     ih_Hook;
    uint32_t *      ih_CPUDone;
    uint32_t *      ih_CPURequested;
};

#endif /* __KERNEL_IPI_H_ */
