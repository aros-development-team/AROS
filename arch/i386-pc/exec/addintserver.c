/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add interrupt client to chain of interrupt server
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#include <proto/exec.h>
#include <aros/libcall.h>

void AndIMask(ULONG);

/*****************************************************************************

    NAME */

	AROS_LH2(void, AddIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 28, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	This function also enables the corresponding chipset interrupt if
	run on a native Amiga (or PC now).

	IMPORTANT!!!
	Adding 0x80000000 to intNumber forces AddIntServer to use real
	PC int number.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ULONG intNum;

    ULONG TransTable[16]=
    {
	0xffffffff,	// INTB_TBE		-> NULL
	0xffffffff,	// INTB_DSKBLK		-> NULL
	0xffffffff,	// INTB_SOFTINT		-> NULL
	0xffffffff,	// INTB_PORTS		-> NULL
	0xffffffff,	// INTB_COPER 		-> NULL
	0,		// INTB_VERTB 		-> IRQ 0
	0xffffffff,	// INTB_BLIT		-> NULL
	0xffffffff,	// INTB_AUD0		-> NULL
	0xffffffff,	// INTB_AUD1		-> NULL
	0xffffffff,	// INTB_AUD2		-> NULL
	0xffffffff,	// INTB_AUD3		-> NULL
	0xffffffff,	// INTB_RBF		-> NULL
	0xffffffff,	// INTB_DSKSYNC		-> NULL
	0xffffffff,	// INTB_EXTER		-> NULL
	0xffffffff,	// INTB_INTEN		-> NULL
	0xffffffff
    };

    Disable();

    if (intNumber & 0x80000000)
    	intNum=intNumber & 0x7fffffff;
    else
    	intNum=TransTable[intNumber];	

    if (intNum!=0xffffffff)
    {
        Enqueue((struct List *)SysBase->IntVects[intNum].iv_Data, (struct Node *)interrupt);
        AndIMask(~(1<<intNum));
    }
    
    Enable();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
