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
	The given Interrupr structure is inserted into list according to
	its proirity. The callback code is called with the following arguments:
	
	    A0 - scratch (set to NULL)
	    A6 - struct ExecBase *
	    A1 - is_Data

    INPUTS
	interrupt - A pointer to an Interrupt structure

    RESULT
	TRUE for success, FALSE for failure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	On AROS this function can not fail. Return value is present for
	AmigaOS v4 compatibility.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntExecBase *IntSysBase = (struct IntExecBase *)SysBase;

    Enqueue(&IntSysBase->ResetHandlers, &interrupt->is_Node);
    return TRUE;

    AROS_LIBFUNC_EXIT
}
