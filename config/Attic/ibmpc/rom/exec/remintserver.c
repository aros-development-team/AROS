/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Remove an interrupt handler.
    Lang:
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#include <proto/exec.h>
#include <aros/libcall.h>

void OrIMask(ULONG);

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

    list = (struct List *)SysBase->IntVects[intNumber].iv_Data;

    Disable();

    Remove((struct Node *)interrupt);

    if(list->lh_TailPred == (struct Node *)list)
    {
	/* disable interrupts if there are no more nodes on the list */
	OrIMask(1<<intNumber);
    }

    Enable();

    AROS_LIBFUNC_EXIT
} /* RemIntServer */
