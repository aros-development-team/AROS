/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, InitCode,

/*  SYNOPSIS */
	AROS_LHA(unsigned long, startClass, D0),
	AROS_LHA(unsigned long, version, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 12, Exec)

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    aros_print_not_implemented ("InitCode");

    AROS_LIBFUNC_EXIT
} /* InitCode */
