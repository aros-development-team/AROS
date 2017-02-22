/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search for a port by name.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

	AROS_LH1(struct MsgPort *, FindPort,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 65, Exec)

/*  FUNCTION
	Look for a public messageport by name. This function doesn't
	arbitrate for the port list and must be protected with a Forbid()
	Permit() pair.

    INPUTS
	port - Pointer to NUL terminated C string.

    RESULT
	Pointer to struct MsgPort or NULL if there is no port of that name.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *retVal;

    /* Nothing spectacular - just look for that name. */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->PortListSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    retVal = (struct MsgPort *)FindName(&SysBase->PortList,name);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->PortListSpinLock);
#endif

    return retVal;

    AROS_LIBFUNC_EXIT
} /* FindPort */

