/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

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

    ReleaseSemaphore (GetPrivIBase(IntuitionBase)->SigSem);

    AROS_LIBFUNC_EXIT
} /* UnlockIBase */
