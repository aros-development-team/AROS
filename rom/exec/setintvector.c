/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Install an interrupt handler.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <aros/libcall.h>

#include "exec_debug.h"

/*****************************************************************************

    NAME */

	AROS_LH2(struct Interrupt *, SetIntVector,

/*  SYNOPSIS */
	AROS_LHA(ULONG,              intNumber, D0),
	AROS_LHA(struct Interrupt *, interrupt, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 27, Exec)

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
    struct Interrupt *oldint;
    BOOL ishandler = intNumber <= 2 || (intNumber >= 6 && intNumber <= 12);

    ExecLog(SysBase, EXECDEBUGF_EXCEPTHANDLER, "SetIntVector: Int %d, Interrupt %p\n", intNumber, interrupt);

    Disable ();

    oldint = (struct Interrupt *)SysBase->IntVects[intNumber].iv_Node;
    SysBase->IntVects[intNumber].iv_Node = ishandler ? (struct Node *)interrupt : NULL;

    if (interrupt)
    {
	SysBase->IntVects[intNumber].iv_Data = interrupt->is_Data;
	SysBase->IntVects[intNumber].iv_Code = interrupt->is_Code;
    }
    else
    {
	SysBase->IntVects[intNumber].iv_Data = (APTR)~0;
	SysBase->IntVects[intNumber].iv_Code = (void *)~0;
    }

    Enable ();

    return oldint;

    AROS_LIBFUNC_EXIT
} /* SetIntVector */
