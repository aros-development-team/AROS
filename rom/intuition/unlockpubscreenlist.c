/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.3  1999/08/20 16:38:58  SDuvan
    Public screen functions


    Desc:
    Lang: English
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(VOID, UnlockPubScreenList,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 88, Intuition)

/*  FUNCTION

    Release lock made by LockPubScreenList().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    LockPubScreenList()

    INTERNALS

    HISTORY
        21-06-98    SDuvan  Implemented
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
#define GPB(x) GetPrivIBase(x)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ReleaseSemaphore(&GPB(IntuitionBase)->PubScrListLock);

    AROS_LIBFUNC_EXIT
} /* UnlockPubScreenList */

