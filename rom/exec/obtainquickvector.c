/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(ULONG, ObtainQuickVector,

/*  SYNOPSIS */
	__AROS_LA(APTR, interruptCode, A0),

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

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)
    __AROS_FUNC_EXIT
} /* ObtainQuickVector */
