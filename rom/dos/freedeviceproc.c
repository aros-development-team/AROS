/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeDeviceProc() - Clean up after calls to GetDeviceProc()
    Lang: english
*/
#include "dos_intern.h"

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
	dp		- DevProc structure as returned by GetDeviceProc().

    RESULT
	Some memory and other resources returned to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDeviceProc()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    if( dp )
    {
	if( dp->dvp_Flags & DVPF_UNLOCK )
	    UnLock( dp->dvp_Lock );
	FreeMem( dp, sizeof(struct DevProc) );
    }

    AROS_LIBFUNC_EXIT
} /* FreeDeviceProc */
