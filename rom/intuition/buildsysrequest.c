/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function BuildSysRequest()
    Lang: english
*/

#include "intuition_intern.h"
#include <proto/exec.h>

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
	FreeSysRequest(), DisplayAlert(), ModifyIDCMP(), exec-library/Wait(),
	Request(), AutoRequest(), EasyRequest(), BuildEasyRequestArgs()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/BuildSysRequest()
    aros_print_not_implemented ("BuildSysRequest");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* BuildSysRequest */
