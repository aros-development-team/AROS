/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#ifdef _AMIGA
#include <hardware/custom.h>
#endif

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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
#ifdef _AMIGA
    struct Custom *custom = (struct Custom *)((void **)0xdff000);
#endif

    Disable ();

    Enqueue ((struct List *)SysBase->IntVects[intNumber].iv_Data,
	(struct Node *)interrupt);

#ifdef _AMIGA
    custom->intena = (UWORD)(INTF_SETCLR|(1<<intNumber));
#endif

    Enable ();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
