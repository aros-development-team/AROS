/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  2000/07/24 18:58:30  stegerg
    added an ASSERT_VALID_PTR(screen)

    Revision 1.6  1999/01/16 23:18:29  hkiel
    Extended AutoDocs

    Revision 1.5  1998/10/20 16:45:58  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:39  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:04  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:20  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:47  digulla
    New functions to handle Public Screens


    Desc: Intuition function GetScreenDrawInfo()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(struct DrawInfo *, GetScreenDrawInfo,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 115, Intuition)

/*  FUNCTION
	Returns a pointer to struct DrawInfo of the passed screen.
	This data is READ ONLY. The version of the struct DrawInfo
	is given in the dri_Version field.

    INPUTS
	screen - The screen you want to get the DrawInfo from.
		Must be valid and open.

    RESULT
	Returns pointer to struct DrawInfo defined in intuition/screens.h

    NOTES
	Call FreeScreenDrawInfo() after finishing using the pointer.
	This function does not prevent the screen from being closed.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeScreenDrawInfo(), LockPubScreen(), intuition/screens.h

    INTERNALS
	Only returns the pointer.

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ASSERT_VALID_PTR(screen);
    
    return &(((struct IntScreen *)screen)->DInfo);

    AROS_LIBFUNC_EXIT
} /* GetScreenDrawInfo */
