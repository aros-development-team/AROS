/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <proto/exec.h>

#define DEBUG_FREESYSREQUEST(x) ;

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

    struct Screen   	    	*scr;
    struct Gadget   	    	*gadgets;
    STRPTR  	    	     	*gadgetlabels;
    struct IntRequestUserData 	*requserdata;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: window 0x%lx\n", (ULONG) window));

    if ((window == NULL) || (window == (void *)1L))
        return;

    scr = window->WScreen;

    requserdata = (struct IntRequestUserData *)window->UserData;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: requserdata 0x%lx\n", (ULONG) requserdata));

    gadgets = requserdata->Gadgets;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgets 0x%lx\n", (ULONG) gadgets));

    /* Remove gadgets before closing window to avoid conflicts with system gadgets */
    RemoveGList(window, gadgets, requserdata->NumGadgets);

    gadgetlabels = requserdata->GadgetLabels;

    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: gadgetlabels 0x%lx\n", (ULONG) gadgetlabels));

    window->UserData = 0;
    CloseWindow(window);
    intrequest_freegadgets(gadgets, IntuitionBase);
    intrequest_freelabels(gadgetlabels, IntuitionBase);

#ifdef SKINS
    DEBUG_FREESYSREQUEST(dprintf("intrequest_freesysrequest: freeitext 0x%lx\n", (ULONG) requserdata->freeitext));
    if (requserdata->freeitext) intrequest_freeitext(requserdata->Text,IntuitionBase);
    if (requserdata->backfilldata.image) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
    if (requserdata->Logo) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
    if (requserdata->ReqGadgets) FreeVec(requserdata->ReqGadgets);
    if (requserdata->dri) FreeScreenDrawInfo(requserdata->ReqScreen,(struct DrawInfo *)requserdata->dri);
#endif
    FreeVec(requserdata);

    return;
    AROS_LIBFUNC_EXIT
} /* FreeSysRequest */
