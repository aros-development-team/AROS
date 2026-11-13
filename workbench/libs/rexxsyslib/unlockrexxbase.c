/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
        Releases the RexxSys library semaphore acquired with
        LockRexxBase().

    INPUTS
        resource - Ignored by this implementation.

    RESULT
        None.

    NOTES
        The calling task must hold the corresponding lock acquired with
        LockRexxBase(). Pair every successful acquisition with a release.

    SEE ALSO
        LockRexxBase()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ReleaseSemaphore(&RSBI(RexxSysBase)->semaphore);

    ReturnVoid("UnlockRexxBase");
    AROS_LIBFUNC_EXIT
} /* UnlockRexxBase */
