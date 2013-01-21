/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <strings.h>

#include "exec_intern.h"
#include "etask.h"

#include "kernel_intern.h"
#include "kernel_arch.h"
#include "kernel_romtags.h"

extern void GPUSysTimerHandler(unsigned int, void *);
extern void *KrnAddSysTimerHandler(uint8_t, irqhandler_t *, void *, void *);

static int PlatformInit(struct KernelBase *KernelBase)
{
    unsigned int stc;
    D(bug("[Kernel] PlatformInit()\n"));

    stc = *((volatile unsigned int *)(SYSTIMER_CLO));
    stc += VBLANK_INTERVAL;
    *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << 3);
    *((volatile unsigned int *)(SYSTIMER_C0 + (VBLANK_TIMER * 4))) = stc;

    KrnAddSysTimerHandler(IRQ_TIMER0 + VBLANK_TIMER, GPUSysTimerHandler, VBLANK_TIMER, KernelBase);

    D(bug("[Kernel] SysTimer %d used for VBlank\n", VBLANK_TIMER));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)

struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)KernelBase;
}
