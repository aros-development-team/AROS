/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Permits the access to all public fields of the IntuitionBase
    Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, UnlockIBase,

/*  SYNOPSIS */
	AROS_LHA(ULONG, ibLock, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 70, Intuition)

/*  FUNCTION
	Release parts of Intuition which have been blocked with a prior
	call to LockIBase().

    INPUTS
	ibLock - The result of LockIBase().

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	LockIBase()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ReleaseSemaphore (GetPrivIBase(IntuitionBase)->IBaseLock);

    AROS_LIBFUNC_EXIT
} /* UnlockIBase */
