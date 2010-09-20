/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch() - Switch to the next available task.
    Lang: english
*/

#include <exec/execbase.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Switch,

/*  LOCATION */
	struct ExecBase *, SysBase, 9, Exec)

/*  FUNCTION
	This function is obsolete and subject to removal.
	On AmigaOS(tm) this was a private function.

    INPUTS

    RESULT

    NOTES
	This function is still there because i386-native port
	still uses it.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch(), Reschedule()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
} /* Switch() */
