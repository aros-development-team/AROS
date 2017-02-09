/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

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

    ForeachNode(intList, irq) {
        if (AROS_INTC3(irq->is_Code, irq->is_Data, intMask, custom)) {
#ifndef __mc68000
            ret = TRUE;
            break;
#endif
         }
    }

    return ret;

    AROS_INTFUNC_EXIT
}
