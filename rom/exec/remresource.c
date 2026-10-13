/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: Removes a resource from the list of public resources.
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_locks.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, RemResource,

/*  SYNOPSIS */
        AROS_LHA(APTR, resource,A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 82, Exec)

/*  FUNCTION
        Removes a resource from the system resource list.

    INPUTS
        resource - Pointer to the resource.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AddResource(), OpenResource()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the resource list */
    EXEC_LOCK_LIST_WRITE_AND_FORBID(&SysBase->ResourceList);

    Remove((struct Node *)resource);

    /* All done. */
    EXEC_UNLOCK_LIST_AND_PERMIT(&SysBase->ResourceList);
    AROS_LIBFUNC_EXIT
} /* RemResource */
