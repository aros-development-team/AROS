#ifndef KERNEL_INTR_H
#define KERNEL_INTR_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "exec_platform.h"

#if defined(__AROSEXEC_SMP__)
/* krnIPIPortThisCPU(), where the port has more than one CPU */
#include <kernel_ipicall.h>
#endif

/* Main scheduler entry points */
void core_HaltCPU(void) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs);
void core_SysCall(int sc, regs_t *regs);

/* CPU-specific wrappers. Need to be implemented in CPU-specific parts */
void cpu_Switch(regs_t *regs);
void cpu_Dispatch(regs_t *regs);

/* This constant can be redefined in arch-specific includes */
#ifndef _CUSTOM
#define _CUSTOM NULL
#endif

/* Call exec interrupt vector, if present */
static inline void core_Cause(unsigned char n, unsigned int mask)
{
    struct IntVector *iv = &SysBase->IntVects[n];

    /* If the SoftInt vector in SysBase is set, call it. It will do the rest for us */
    if (iv->iv_Code)
        AROS_INTC3(iv->iv_Code, iv->iv_Data, mask, _CUSTOM);
}

/*
 * KrnCli() by its entry point. It is called here on every way out of an
 * interrupt, in files that reach KernelBase in different ways, or do not
 * have it yet; the function itself does not use its base.
 */
AROS_LD0I(void, KrnCli, struct KernelBase *, KernelBase, 9, Kernel);

/*
 * Run the software interrupts that are pending. This is the lowest
 * interrupt level: it is called where a CPU is about to leave the last
 * hardware interrupt for a task, or wakes in its idle loop, and it
 * returns with interrupts masked and nothing left for this CPU to run,
 * so that whatever comes next (the scheduler, the dispatcher) is decided
 * with interrupts masked, as on the original.
 *
 * The handlers run with interrupts enabled, and so does the end of exec's
 * SoftIntDispatch(). A Cause() made by a hardware interrupt that arrives
 * after its last look at the lists has set SFF_SoftInt again: the flag is
 * tested with interrupts masked, and nothing can set it on this CPU
 * between that test and the return.
 *
 * On SMP only the boot CPU runs them, so they never run beside each
 * other. Cause() on another CPU sends the boot CPU here.
 */
static inline void core_RunSoftInts(void)
{
    for (;;)
    {
        AROS_SLIB_ENTRY(KrnCli, Kernel, 9)();

        if (!(SysBase->SysFlags & SFF_SoftInt))
            break;
#if defined(__AROSEXEC_SMP__) && defined(KRN_IPICALL)
        if (krnIPIPortThisCPU() != 0)
            break;
#endif
        /* Nothing would take the request: do not wait for it */
        if (!SysBase->IntVects[INTB_SOFTINT].iv_Code)
            break;

        core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
    }
}

/* Call exec trap handler, if possible */
static inline int core_Trap(ULONG code, void *regs)
{
    /* exec.library Alert() is inoperative without KernelBase.
     * SysBase is setup before KernelBase is available. Check for
     * SysBase->KernelBase before calling this function
     */
    if (SysBase)
    {
        void (*trapHandler)(ULONG, void *) = SysBase->TaskTrapCode;
        struct Task *t = GET_THIS_TASK;

        if ((t) && (t->tc_TrapCode))
            trapHandler = t->tc_TrapCode;

        if (trapHandler)
        {
            trapHandler(code, regs);
            return 1;
        }
    }
    return 0;
}
#endif /* !KERNEL_INTR_H */
