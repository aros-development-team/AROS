/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: Obtain and install a Quick Interrupt vector
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

    return 0L;
    AROS_LIBFUNC_EXIT
} /* ObtainQuickVector */
