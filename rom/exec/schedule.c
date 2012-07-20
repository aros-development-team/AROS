/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(void, Schedule,

/*  SYNOPSIS */

/*  LOCATION */
        struct ExecBase *, SysBase, 7, Exec)

/*  FUNCTION
        PRIVATE architecture specific routine for relinquishing CPU time

    INPUTS
        None

    RESULT
        None

    NOTES
        This function was private in AmigaOS(tm) up to v3.1. There's no guarantee
        that it will continue to exist in other systems.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_FUNCTION_NOT_IMPLEMENTED("Exec");

    AROS_LIBFUNC_EXIT
} /* Schedule */
