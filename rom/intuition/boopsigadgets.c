/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Routines for BOOL Gadgets
    Lang: english
*/

#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>


#include "intuition_intern.h"
#undef DEBUG
#define DEBUG 0
#	include <aros/debug.h>

VOID DoGMLayout(struct Gadget		*glist,
		struct Window		*win,
		struct Requester	*req,
		UWORD			numgad,
		BOOL			initial,
		struct IntuitionBase	*IntuitionBase)
{
    
    while (glist && numgad)
    {
    	/* Is this a BOOPSI gad with special relativity ? */
    	if (((glist->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) &&
	    (glist->Flags & (GFLG_RELSPECIAL | GFLG_RELRIGHT | GFLG_RELBOTTOM |
	                     GFLG_RELWIDTH | GFLG_RELHEIGHT)))
    	{
    	    struct gpLayout lmsg;
    	    lmsg.MethodID    = GM_LAYOUT;
    	    lmsg.gpl_GInfo   = NULL;
    	    lmsg.gpl_Initial = initial;

    	    DoGadgetMethodA(glist, win, req, (Msg)&(lmsg));

    	}
    	
        glist = glist->NextGadget;
    	numgad --;

    }
    return;
}


void RefreshBoopsiGadget (struct Gadget * gadget, struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
    struct gpRender gpr;

    gpr.MethodID   = GM_RENDER;
    gpr.gpr_Redraw = GREDRAW_REDRAW;

    DoGadgetMethodA (gadget, win, NULL, (Msg)&gpr);
} /* RefreshBoopsiGadget */
