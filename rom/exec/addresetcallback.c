/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a reset callback to internal list
    Lang: english
*/

#include <exec/interrupts.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(BOOL, AddResetCallback,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, interrupt, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 167, Exec)

/*  FUNCTION
	Install a system reset notification callback. The callback
	will be called whenever system reboot is performed.

	The given Interrupt structure is inserted into the callback list
	according to its priority. The callback code is called with the same
	arguments as an interrupt server.

    INPUTS
	interrupt - A pointer to an Interrupt structure

    RESULT
	TRUE for success, FALSE for failure

    NOTES
	This function is compatible with AmigaOS v4.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	On AROS this function cannot fail. Return value is present for
	AmigaOS v4 compatibility.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntExecBase *IntSysBase = (struct IntExecBase *)SysBase;

    Enqueue(&IntSysBase->ResetHandlers, &interrupt->is_Node);
    return TRUE;

    AROS_LIBFUNC_EXIT
}
