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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    Disable();

    Enqueue((struct List *)SysBase->IntVects[intNumber].iv_Data, (struct Node *)interrupt);

    AndIMask(~(1<<intNumber));

    Enable();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
