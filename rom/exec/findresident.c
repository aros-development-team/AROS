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

	AROS_LH1(struct Resident *, FindResident,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 16, Exec)

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

    aros_print_not_implemented ("FindResident");

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindResident */
