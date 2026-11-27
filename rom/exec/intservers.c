/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#if defined(__AROSEXEC_SMP__)
/* PrivExecBase() and the lock on the server chains */
#include "exec_intern.h"
#endif

#include "intservers.h"

/*
 * Our default IntVectors.
 * This interrupt handler will send an interrupt to a series of queued
 * interrupt servers. Servers should return D0 != 0 (Z clear) if they
 * believe the interrupt was for them, and no further interrupts will
 * be called. This will only check the value in D0 for non-m68k systems,
 * however it SHOULD check the Z-flag on 68k systems.

 * Hmm, in that case I would have to separate it from this file in order
 * to replace it... TODO: this can be done after merging exec_init.c from
 * i386 and PPC native.
*/
AROS_INTH3(IntServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

    struct Interrupt * irq;
    BOOL ret = FALSE;

#if defined(__AROSEXEC_SMP__)
    /*
     * Disable() in AddIntServer() and RemIntServer() only stops the CPU
     * they are called on; this chain may be walked on another one. Hold
     * the lock they take, in read mode, for the whole walk: the chain
     * does not change under us, and RemIntServer() does not return while
     * the server it removes is still running here, so its caller can free
     * it. We are in interrupt context, so the spinlock is taken directly,
     * as SoftIntDispatch() does, and not through the execlock functions,
     * which Disable() and Enable().
     *
     * Consequence, the same as for a kernel IRQ handler: a server must
     * not call AddIntServer(), RemIntServer() or SetIntVector() for a
     * vector below INTB_KERNEL from within itself. That would wait for
     * the write lock while this CPU holds the read lock.
     */
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->IntrListSpinLock, NULL, SPINLOCK_MODE_READ);
#endif

    ForeachNode(intList, irq) {
        if (AROS_INTC3(irq->is_Code, irq->is_Data, intMask, custom)) {
#ifndef __mc68000
            ret = TRUE;
            break;
#endif
         }
    }

#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->IntrListSpinLock);
#endif

    return ret;

    AROS_INTFUNC_EXIT
}
