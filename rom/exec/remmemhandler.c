/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a memory handler.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemMemHandler,

/*  SYNOPSIS */
	AROS_LHA(struct Interrupt *, memHandler, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 130, Exec)

/*  FUNCTION
	Remove some function added with AddMemHandler again.

    INPUTS
	memHandler - The same Interrupt structure you gave to AddMemHandler().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Protect the low memory handler list */
    ObtainSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    /* Nothing spectacular: Just remove node */
    Remove(&memHandler->is_Node);

    ReleaseSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    AROS_LIBFUNC_EXIT
} /* RemMemHandler */

