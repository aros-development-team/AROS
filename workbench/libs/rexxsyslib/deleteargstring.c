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

	AROS_LH1(VOID, DeleteArgstring,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, argstring, A0),

/*  LOCATION */
	struct Library *, RexxSysBase, 22, RexxSys)

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

    aros_print_not_implemented ("DeleteArgstring");

    AROS_LIBFUNC_EXIT
} /* DeleteArgstring */
