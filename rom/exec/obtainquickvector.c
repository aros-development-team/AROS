/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain and install a Quick Interrupt vector
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, ObtainQuickVector,

/*  SYNOPSIS */
	AROS_LHA(APTR, interruptCode, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 131, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

#warning TODO: Write exec/ObtainQuickVector()
    aros_print_not_implemented ("ObtainQuickVector");

    return 0L;
    AROS_LIBFUNC_EXIT
} /* ObtainQuickVector */
