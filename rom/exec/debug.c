/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: Internal debugger.
*/

#include <aros/debug.h>
#include <exec/interrupts.h>
#include <libraries/debug.h>
#include <proto/exec.h>
#include <proto/debug.h>

#include <ctype.h>
#include <string.h>

#include "debug_internal.h"
#include "exec_intern.h"
#include "exec_util.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, Debug,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 19, Exec)

/*  FUNCTION
        Runs SAD - internal debuger.

    INPUTS
        flags   not used. Should be 0 now.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        18-01-99    initial PC version.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    InternalDebug(NULL);

    AROS_LIBFUNC_EXIT
} /* Debug */
