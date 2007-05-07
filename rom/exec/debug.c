/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start the internal debugger.
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, Debug,

/*  SYNOPSIS */
	AROS_LHA(unsigned long, flags, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 19, Exec)

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

#warning TODO: Write exec/Debug()
    aros_print_not_implemented ("Debug");

    AROS_LIBFUNC_EXIT
} /* Debug */
