/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  1999/01/16 23:19:48  hkiel
    Added aros_print_not_implemented()

    Revision 1.5  1998/10/20 16:45:55  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:38  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:03  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:19  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:48  digulla
    New functions to handle Public Screens


    Desc: Intuition function FreeScreenDrawInfo()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, FreeScreenDrawInfo,

/*  SYNOPSIS */
	AROS_LHA(struct Screen   *, screen, A0),
	AROS_LHA(struct DrawInfo *, drawInfo, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 116, Intuition)

/*  FUNCTION
	Tell intuition that you have finished work with struct DrawInfo
	returned by GetScreenDrawInfo()

    INPUTS
	screen - The screen you passed to GetScreenDrawInfo()
	drawInfo - The DrawInfo structure returned by GetScreenDrawInfo()

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetScreenDrawInfo()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/FreeScreenDrawInfo()
    aros_print_not_implemented ("FreeScreenDrawInfo");

    AROS_LIBFUNC_EXIT
} /* FreeScreenDrawInfo */
