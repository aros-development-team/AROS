/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/10 14:00:05  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:47  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <clib/intuition_protos.h>

	AROS_LH1(struct Screen *, LockPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 85, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (!name)
	return GetPrivIBase(IntuitionBase)->WorkBench;

    return NULL;
    AROS_LIBFUNC_EXIT
} /* LockPubScreen */
