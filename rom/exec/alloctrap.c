/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate a trap
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, AllocTrap,

/*  SYNOPSIS */
	AROS_LHA(long, trapNum, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 57, Exec)

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

#warning TODO: Write exec/AllocTrap()
    aros_print_not_implemented ("AllocTrap");

    return -1L;
    AROS_LIBFUNC_EXIT
} /* AllocTrap */
