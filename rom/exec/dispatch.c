/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dispatch() - Tell the system that we have switched tasks.
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <exec/alerts.h>

#include <proto/arossupport.h>
#include <aros/asmcall.h>

#include "etask.h"

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Dispatch,

/*  LOCATION */
	struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
	This function is obsolete and subject to removal.
	On AmigaOS(tm) this was a private function.

    INPUTS
	None.

    RESULT

    NOTES
	This function is still there because i386-native port
	still uses it.

    EXAMPLE

    BUGS

    SEE ALSO
	Switch(), Reschedule()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
} /* Dispatch() */
