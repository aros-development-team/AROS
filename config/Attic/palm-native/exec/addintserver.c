/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add interrupt client to chain of interrupt server
    Lang: english
*/

#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>

#include <proto/exec.h>
#include <aros/libcall.h>

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

    Disable();

    Enqueue((struct List *)SysBase->IntVects[intNumber].iv_Data, (struct Node *)interrupt);

    Enable();

    AROS_LIBFUNC_EXIT
} /* AddIntServer */
