/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
*/
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

        AROS_LH1(VOID, LockRexxBase,

/*  SYNOPSIS */
        AROS_LHA(ULONG, resource, D0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 75, RexxSys)

/*  FUNCTION
        Acquires the RexxSys library semaphore, waiting until it becomes
        available.

    INPUTS
        resource - Ignored by this implementation.

    RESULT
        None.

    NOTES
        Release the semaphore with UnlockRexxBase() after accessing the
        shared RexxSys state. Synchronization applies only to code using
        the same semaphore.

    SEE ALSO
        UnlockRexxBase()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&RSBI(RexxSysBase)->semaphore);
  
    ReturnVoid("LockRexxBase");
    AROS_LIBFUNC_EXIT
} /* LockRexxBase */
