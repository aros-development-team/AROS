/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadGayle() - get the Gayle ID
    Lang: english
*/
#include <aros/debug.h>

#include "exec_intern.h"

/*****************************************************************************
*/

#include <proto/exec.h>

/*  NAME */

    AROS_LH0(ULONG, ReadGayle,

/*  LOCATION */
	struct ExecBase *, SysBase, 136, Exec)

/*  FUNCTION
    Gets the Gayle ID

    INPUTS
	None.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
