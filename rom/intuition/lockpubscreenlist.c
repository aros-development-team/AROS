/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.2  1999/08/20 16:38:58  SDuvan
    Public screen functions


    Desc:
    Lang: English
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(struct List *, LockPubScreenList,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 87, Intuition)

/*  FUNCTION

    Arbitrates access to the system public screen list. This is for Public
    Screen Manager programs only! The list should be locked as short a time
    as possible.

    INPUTS

    RESULT

    NOTES

    The list's nodes are PubScreenNodes as defined in <intuition/screens.h>.
    Act very quickly when holding this lock!

    EXAMPLE

    BUGS

    SEE ALSO

    UnlockPubScreenList()

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

    ObtainSemaphore(&GPB(IntuitionBase)->PubScrListLock);
    return (struct List *)&GPB(IntuitionBase)->PubScreenList;

    AROS_LIBFUNC_EXIT
} /* LockPubScreenList */
