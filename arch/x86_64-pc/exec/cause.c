/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: cause.c 22006 2004-08-05 19:06:53Z stegerg $

    Desc: i386native version of Cause().
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <exec_intern.h>

AROS_LH1(void, Cause,
    AROS_LHA(struct Interrupt *, softint, A1),
    struct ExecBase *, SysBase, 30, Exec)
{
    AROS_LIBFUNC_INIT

    UBYTE pri;
    Disable();
    /* Check to ensure that this node is not already in a list. */
    if( softint->is_Node.ln_Type != NT_SOFTINT )
    {
        /* Scale the priority down to a number between 0 and 4 inclusive
        We can use that to index into exec's software interrupt lists. */
        pri = (softint->is_Node.ln_Pri + 0x20)>>4;

        /* We are accessing an Exec list, protect ourselves. */
        ADDTAIL(&SysBase->SoftInts[pri].sh_List, &softint->is_Node);
        softint->is_Node.ln_Type = NT_SOFTINT;
        SysBase->SysFlags |= SFF_SoftInt;

        /* If we are in usermode the software interrupt will end up
           being triggered in Enable(). See Enable() code */
    }
    Enable();


    AROS_LIBFUNC_EXIT
} /* Cause() */
