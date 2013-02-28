/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeDeviceProc() - Clean up after calls to GetDeviceProc()
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

        AROS_LH1(void, FreeDeviceProc,

/*  SYNOPSIS */
        AROS_LHA(struct DevProc *, dp, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 108, Dos)

/*  FUNCTION
        FreeDeviceProc() will clean up after a call to GetDeviceProc().

    INPUTS
        dp - DevProc structure as returned by GetDeviceProc(), or NULL.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if( dp )
    {
        if( dp->dvp_Flags & DVPF_UNLOCK )
            UnLock( dp->dvp_Lock );
        FreeMem( dp, sizeof(struct DevProc) );
    }

    AROS_LIBFUNC_EXIT
} /* FreeDeviceProc */
