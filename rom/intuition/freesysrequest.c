/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function FreeSysRequest()
    Lang: english
*/

#include "intuition_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

	AROS_LH1(void, FreeSysRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 62, Intuition)

/*  FUNCTION
	Frees a requester made with BuildSysRequest() or
	BuildEasyRequestArgs().

    INPUTS
	Window - The requester to be freed. May be NULL or 1.

    RESULT

    NOTES

    EXAMPLE

    BUGS
	BuildSysRequest() requesters not supported, yet.

    SEE ALSO
	BuildSysRequest(), BuildEasyRequestArgs()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *scr;
    struct Gadget *gadgets;
    STRPTR *gadgetlabels;
    
    struct IntRequestUserData *requserdata;

    if ((window == NULL) || (window == (void *)1L))
	return;

    scr = window->WScreen;
    
    requserdata = (struct IntRequestUserData *)window->UserData;
    gadgets = requserdata->Gadgets;
    
    /* Remove gadgets before closing window to avoid conflicts with system gadgets */
    RemoveGList(window, gadgets, requserdata->NumGadgets);
    
    gadgetlabels = requserdata->GadgetLabels;

    FreeVec(window->UserData);
    CloseWindow(window);
    intrequest_freegadgets(gadgets, IntuitionBase);
    intrequest_freelabels(gadgetlabels, IntuitionBase);

    return;
    AROS_LIBFUNC_EXIT
} /* FreeSysRequest */
