/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove an interrupt handler.
    Lang:
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#include <hardware/custom.h>
#include <hardware/intbits.h>
#endif

#include <proto/exec.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, RemIntServer,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 29, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct List *list;
#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    struct Custom *custom = (struct Custom *)(void **)0xdff000;
#endif

    list = (struct List *)SysBase->IntVects[intNumber].iv_Data;

    Disable();

    Remove((struct Node *)interrupt);

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    if(list->lh_TailPred == (struct Node *)list)
    {
	/* disable interrupts if there are no more nodes on the list */
	custom->intena = (UWORD)((1<<intNumber));
    }
#endif

    Enable();

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
