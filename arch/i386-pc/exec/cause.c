/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

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

#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <asm/irq.h>

#include "etask.h"

extern char softblock;

#define get_cs() \
	({  short __value; \
	__asm__ __volatile__ ("mov %%ds,%%ax":"=a"(__value)); \
	(__value & 0x03);	})

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
#if 0
	if (!softblock)
            if (get_cs())
                __asm__ __volatile__ ("movl $0,%%eax\n\tint $0x80":::"eax","memory");
             else
             {
             /* Cause() inside supervisor mode. We will call IntServer directly
                no matter it is normal irq or Supervisor() call */
             struct IntVector *iv = &SysBase->IntVects[INTB_SOFTINT];

                 if (iv->iv_Code)
                 {
                     AROS_UFC5(void, iv->iv_Code,
                         AROS_UFCA(ULONG, 0, D1),
                         AROS_UFCA(ULONG, 0, A0),
                         AROS_UFCA(APTR, NULL, A1),
                         AROS_UFCA(APTR, iv->iv_Code, A5),
                         AROS_UFCA(struct ExecBase *, SysBase, A6)
                     );
                 }
             }
#endif
	     
    }
    Enable();


    AROS_LIBFUNC_EXIT
} /* Cause() */

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (*(struct ExecBase **)4UL)

/*
#define SC_ENABLE(regs)		(regs.eflags |= 0x200)
#define SC_DISABLE(regs)	(regs.eflags &= ~0x200)
*/

// asmlinkage void sys_Cause(struct pt_regs regs)
asmlinkage void sys_Cause(void)
{
    struct IntVector *iv;

    iv = &SysBase->IntVects[INTB_SOFTINT];

    if (iv->iv_Code)
    {
        /*  Call it. I call with all these parameters for a reason.

        In my `Amiga ROM Kernel Reference Manual: Libraries and
        Devices' (the 1.3 version), interrupt servers are called
        with the following 5 parameters.

        D1 - Mask of INTENAR and INTREQR
        A0 - 0xDFF000 (base of custom chips)
        A1 - Interrupt Data
        A5 - Interrupt Code vector
        A6 - SysBase

        It is quite possible that some code uses all of these, so
        I must supply them here. Obviously I will dummy some of these
        though.
	*/
        AROS_UFC5(void, iv->iv_Code,
            AROS_UFCA(ULONG, 0, D1),
            AROS_UFCA(ULONG, 0, A0),
            AROS_UFCA(APTR, 0, A1),
            AROS_UFCA(APTR, iv->iv_Code, A5),
            AROS_UFCA(struct ExecBase *, SysBase, A6)
        );
    }
}

#undef SysBase

/*
    This is the dispatcher for software interrupts. We go through the
    lists and remove the interrupts before calling them. Because we
    can be interrupted we should not cache the next pointer, but
    retreive it from ExecBase each time.

    Note: All these arguments are passed to the structure, so you must
    at least declare all of them. You do not however have to use any
    of them (although that is recommended).

    This procedure could be more efficient, and it has to be implemented
    in the kernel.
*/

AROS_UFH5(void, SoftIntDispatch,
    AROS_UFHA(ULONG, intReady, D1),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(IPTR, intData, A1),
    AROS_UFHA(IPTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt *intr = 0;
    BYTE i;

    if( SysBase->SysFlags & SFF_SoftInt )
    {
#if 1
        /* Clear the Software interrupt pending flag. */
        SysBase->SysFlags &= ~(SFF_SoftInt);

    	for(;;)
	{
            for(i=4; i>=0; i--)
            {
	    	__cli();
        	intr = (struct Interrupt *)RemHead(&SysBase->SoftInts[i].sh_List);
		
		if (intr)		
        	{
        	    intr->is_Node.ln_Type = NT_INTERRUPT;

    	    	    __sti();
		    
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
	
	
#else
        /* Disable software interrupts */
        softblock = 1;

        /* Clear the Software interrupt pending flag. */
        SysBase->SysFlags &= ~(SFF_SoftInt);

        for(i=4; i>=0; i--)
        {
            while( (intr = (struct Interrupt *)RemHead(&SysBase->SoftInts[i].sh_List)) )
            {
        	intr->is_Node.ln_Type = NT_INTERRUPT;

        	/* Call the software interrupt. */
        	AROS_UFC3(void, intr->is_Code,
                AROS_UFCA(APTR, intr->is_Data, A1),
                AROS_UFCA(APTR, intr->is_Code, A5),
                AROS_UFCA(struct ExecBase *, SysBase, A6));
            }
        }

        /* We now re-enable software interrupts. */
        softblock = 0;
#endif
	
    }
    return;
    AROS_USERFUNC_EXIT
}
