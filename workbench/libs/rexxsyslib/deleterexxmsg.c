/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

	AROS_LH1(VOID, DeleteRexxMsg,

/*  SYNOPSIS */
	AROS_LHA(struct RexxMsg *, packet, A0),

/*  LOCATION */
	struct Library *, RexxSysBase, 25, RexxSys)

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
    AROS_LIBBASE_EXT_DECL(struct Library *,RexxSysBase)

    aros_print_not_implemented ("DeleteRexxMsg");

    AROS_LIBFUNC_EXIT
} /* DeleteRexxMsg */
