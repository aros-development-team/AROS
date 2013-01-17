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

/* Use system timer 3 for our scheduling heartbeat */
#define VBLANK_TIMER 3

static int PlatformInit(struct KernelBase *KernelBase)
{
    unsigned int stc;
    D(bug("[Kernel] PlatformInit()\n"));

    // Let the fun begin ;)
    // clock operates at 1000000hz
    stc = *((volatile unsigned int *)(SYSTIMER_CLO));
    stc += 20000; // (1000000 / 50)
    *((volatile unsigned int *)(SYSTIMER_C0 + (VBLANK_TIMER * 4))) = stc;
    *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << VBLANK_TIMER);

    ictl_enable_irq(IRQ_TIMER0 + VBLANK_TIMER, KernelBase);

    D(bug("[Kernel] VBlank timer enabled\n"));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)

struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)KernelBase;
}
