/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  1999/09/30 19:45:41  stegerg
    implemented

    Revision 1.8  1998/10/20 20:08:03  nlorentz
    Fixed lots of errors due to aros_not_implemented()

    Revision 1.7  1998/10/20 16:45:51  hkiel
    Amiga Research OS

    Revision 1.6  1998/09/12 20:20:08  hkiel
    converted TODO/FIXME comments to #warnings

    Revision 1.5  1997/01/27 00:36:35  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:00  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

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
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH8(BOOL, AutoRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Window    *, window, A0),
	AROS_LHA(struct IntuiText *, body, A1),
	AROS_LHA(struct IntuiText *, posText, A2),
	AROS_LHA(struct IntuiText *, negText, A3),
	AROS_LHA(ULONG             , pFlag, D0),
	AROS_LHA(ULONG             , nFlag, D1),
	AROS_LHA(ULONG             , width, D2),
	AROS_LHA(ULONG             , height, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 58, Intuition)

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

    struct Window *req;
    ULONG idcmp;
    LONG result;
    
    req = BuildSysRequest(window,
    			  body,
			  posText,
			  negText,
			  pFlag | nFlag,
			  width,
			  height);

    /* req = 0/1 is handled by SysReqHandler */
    while ((result = SysReqHandler(req, &idcmp, TRUE)) == -2)
    {
    }

    if (result == -1)
    {
	result = (idcmp & pFlag) ? 1 : 0;
    }

    FreeSysRequest(req);

    return (BOOL)result;

    AROS_LIBFUNC_EXIT
} /* AutoRequest */
