/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/10 14:00:03  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:19  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:48  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <clib/intuition_protos.h>

	AROS_LH2(void, FreeScreenDrawInfo,

/*  SYNOPSIS */
	AROS_LHA(struct Screen   *, screen, A0),
	AROS_LHA(struct DrawInfo *, drawInfo, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 116, Intuition)

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
    AROS_LIBFUNC_EXIT
} /* FreeScreenDrawInfo */
