/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

        AROS_LH0(void, Dispatch,

/*  SYNOPSIS */

/*  LOCATION */
        struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
        PRIVATE function to dispatch next available task

    INPUTS
        None

    RESULT
        None

    NOTES
        This function was private in AmigaOS(tm) up to v3.1.
        There's no guarantee that it will continue to exist in other systems.

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
} /* Dispatch */
