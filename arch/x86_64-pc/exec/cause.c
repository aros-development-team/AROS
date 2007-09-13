/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: cause.c 22006 2004-08-05 19:06:53Z stegerg $

    Desc: i386native version of Cause().
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/kernel.h>

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


AROS_UFH5(void, SoftIntDispatch,
          AROS_UFHA(ULONG, intReady, D1),
          AROS_UFHA(struct Custom *, custom, A0),
          AROS_UFHA(IPTR, intData, A1),
          AROS_UFHA(IPTR, intCode, A5),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    void *KernelBase = TLS_GET(KernelBase);
    
    struct Interrupt *intr = 0;
    BYTE i;

    if( SysBase->SysFlags & SFF_SoftInt )
    {
        /* Clear the Software interrupt pending flag. */
        SysBase->SysFlags &= ~(SFF_SoftInt);

        for(;;)
        {
            for(i=4; i>=0; i--)
            {
                KrnCli();
                intr = (struct Interrupt *)RemHead(&SysBase->SoftInts[i].sh_List);

                if (intr)
                {
                    intr->is_Node.ln_Type = NT_INTERRUPT;

                    KrnSti();

                    /* Call the software interrupt. */
                    AROS_UFC3(void, intr->is_Code,
                              AROS_UFCA(APTR, intr->is_Data, A1),
                              AROS_UFCA(APTR, intr->is_Code, A5),
                              AROS_UFCA(struct ExecBase *, SysBase, A6));

                    /* Get out and start loop *all* over again *from scratch*! */
                    break;
                }
            }

            if (!intr) break;
        }
    }
    AROS_USERFUNC_EXIT
}

