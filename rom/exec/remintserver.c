/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove an interrupt handler.
    Lang:
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(mc68000)
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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(mc68000)
    struct List *list;
    struct Custom *custom = (struct Custom *)(void **)0xdff000;

    list = (struct List *)SysBase->IntVects[intNumber].iv_Data;
#endif

    Disable();

    Remove((struct Node *)interrupt);

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(mc68000)
    if(list->lh_TailPred == (struct Node *)list)
    {
	/* disable interrupts if there are no more nodes on the list */
	custom->intena = (UWORD)((1<<intNumber));
    }
#endif

    Enable();

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
