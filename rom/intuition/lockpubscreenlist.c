/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function LockPubScreenList()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(struct List *, LockPubScreenList,

/*  SYNOPSIS */
	/* VOID */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 87, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/LockPubScreenList()
    aros_print_not_implemented ("LockPubScreenList");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* LockPubScreenList */
