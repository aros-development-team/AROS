/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2000/02/04 21:57:32  stegerg
    protect with MenuLock semaphore.

    Revision 1.8  1999/01/14 15:30:24  hkiel
    Added functions + preliminary code

    Revision 1.7  1998/10/20 20:08:04  nlorentz
    Fixed lots of errors due to aros_not_implemented()

    Revision 1.6  1998/10/20 16:45:53  hkiel
    Amiga Research OS

    Revision 1.5  1998/09/12 20:20:08  hkiel
    converted TODO/FIXME comments to #warnings

    Revision 1.4  1997/01/27 00:36:36  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:00  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:17  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:06:48  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, ClearMenuStrip,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 9, Intuition)

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

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
    window->MenuStrip = NULL;
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    return;

    AROS_LIBFUNC_EXIT
} /* ClearMenuStrip */
