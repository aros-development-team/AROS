/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Build checksum for Kickstart.
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH0(ULONG, SumKickData,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExecBase *, SysBase, 102, Exec)

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

#warning TODO: Write exec/SumKickData()
    aros_print_not_implemented ("SumKickData");

    return 0L;
    AROS_LIBFUNC_EXIT
} /* SumKickData */
