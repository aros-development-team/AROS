/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>

#include "intuition_intern.h"
#ifdef SKINS
#include "intuition_customizesupport.h"
#endif
#undef DEBUG
#define DEBUG 0
#   include <aros/debug.h>

VOID DoGMLayout(struct Gadget *glist, struct Window *win, struct Requester *req,
                UWORD numgad, BOOL initial, struct IntuitionBase *IntuitionBase)
{
    while (glist && numgad)
    {
        /* Is this a BOOPSI gad with special relativity ? */
        if (((glist->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) &&
                (/*initial ||*/
                 (glist->Flags & (GFLG_RELSPECIAL | GFLG_RELRIGHT | GFLG_RELBOTTOM |
                                  GFLG_RELWIDTH | GFLG_RELHEIGHT))))
        {
            struct gpLayout lmsg;
	    
            lmsg.MethodID    = GM_LAYOUT;
            lmsg.gpl_GInfo   = NULL;
            lmsg.gpl_Initial = initial;

            DoGadgetMethodA(glist, win, req, (Msg)&(lmsg));
        }

#ifdef SKINS
        int_relayoutprop(win,glist,IntuitionBase);
#endif
        if (win && !req)
        {
            WORD left   = glist->LeftEdge;
            WORD top    = glist->TopEdge;
            WORD width  = glist->Width;
            WORD height = glist->Height;

            if (glist->Flags & GFLG_RELRIGHT)
                left += win->Width - 1;
            if (glist->Flags & GFLG_RELBOTTOM)
                top += win->Height - 1;
            if (glist->Flags & GFLG_RELWIDTH)
                width += win->Width;
            if (glist->Flags & GFLG_RELHEIGHT)
                height += win->Height;

            if ((left >= win->Width - win->BorderRight ||
                 top >= win->Height - win->BorderBottom ||
                 left +  width - 1 <= win->BorderLeft ||
                 top + height - 1 <= win->BorderTop) &&
                 (glist->Flags & (GFLG_RELSPECIAL | GFLG_RELRIGHT | GFLG_RELBOTTOM |
                                  GFLG_RELWIDTH | GFLG_RELHEIGHT)))
            {
                glist->Activation |= GACT_BORDERSNIFF;
            }
            else
            {
                glist->Activation &= ~GACT_BORDERSNIFF;
            }
        }

        glist = glist->NextGadget;
        numgad --;

    }
    return;
}


void RefreshBoopsiGadget (struct Gadget * gadget, struct Window * win,
                          struct Requester * req, struct IntuitionBase * IntuitionBase)
{
    struct gpRender gpr;

    gpr.MethodID   = GM_RENDER;
    gpr.gpr_Redraw = GREDRAW_REDRAW;

    DoGadgetMethodA (gadget, win, req, (Msg)&gpr);
} /* RefreshBoopsiGadget */
