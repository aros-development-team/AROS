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

	AROS_LH1(VOID, UnlockRexxBase,

/*  SYNOPSIS */
	AROS_LHA(ULONG, resource, D0),

/*  LOCATION */
	struct RxsLib *, RexxSysBase, 76, RexxSys)

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

    ReleaseSemaphore(&RSBI(RexxSysBase)->semaphore);

    ReturnVoid("UnlockRexxBase");
    AROS_LIBFUNC_EXIT
} /* UnlockRexxBase */
