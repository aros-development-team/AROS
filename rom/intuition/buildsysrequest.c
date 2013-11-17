/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "requesters.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

        AROS_LH7(struct Window *, BuildSysRequest,

/*  SYNOPSIS */
        AROS_LHA(struct Window *   , window, A0),
        AROS_LHA(struct IntuiText *, bodytext, A1),
        AROS_LHA(struct IntuiText *, postext, A2),
        AROS_LHA(struct IntuiText *, negtext, A3),
        AROS_LHA(ULONG             , IDCMPFlags , D0),
        AROS_LHA(WORD              , width, D2),
        AROS_LHA(WORD              , height, D3),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 60, Intuition)

/*  FUNCTION
	Build and display a system requester.

    INPUTS
	window - The window in which the requester will appear
	bodytext - The Text to be shown in the body of the requester
	postext - The Text to be shown in the positive choice gadget
	negtext - The Text to be shown in the negative choice gadget
	IDCMPFlags - The IDCMP Flags for this requester
	width, height - The dimensions of the requester

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeSysRequest(), DisplayAlert(), ModifyIDCMP(), exec.library/Wait(),
	Request(), AutoRequest(), EasyRequestArgs(), BuildEasyRequestArgs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return buildsysreq_intern(window, NULL, bodytext, postext, negtext, IDCMPFlags, width, height, IntuitionBase);
    
    AROS_LIBFUNC_EXIT
}
