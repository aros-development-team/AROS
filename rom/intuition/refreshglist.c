/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "boolgadgets.h"
#include "boopsigadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <intuition/classes.h>

BOOL qualifygadget(struct Gadget *gadgets,LONG mustbe, LONG mustnotbe,struct IntuitionBase *IntuitionBase);
void rendergadget(struct Gadget *gadgets,struct Window *window, struct Requester *requester,struct IntuitionBase *IntuitionBase);
struct Gadget *findprevgadget(struct Gadget *gadget,struct Window *window,struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH4(void, RefreshGList,

         /*  SYNOPSIS */
         AROS_LHA(struct Gadget    *, gadgets, A0),
         AROS_LHA(struct Window    *, window, A1),
         AROS_LHA(struct Requester *, requester, A2),
         AROS_LHA(LONG              , numGad, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 72, Intuition)

/*  FUNCTION
    Refresh (draw anew) the specified number of gadgets starting
    at the specified gadget.
 
    INPUTS
    gadgets - This is the first gadget which will be refreshed.
    window - The window which contains the gadget
    requester - If the gadget has GTYP_REQGADGET set, this must be
        a pointer to a Requester; otherwise the value is
        ignored.
    numGad - How many gadgets should be refreshed. The value
        may range from 0 to MAXLONG. If there are less gadgets
        in the list than numGad, only the gadgets in the
        list will be refreshed.
 
    RESULT
    None.
 
    NOTES
    This function *must not* be called inside a
    BeginRefresh()/EndRefresh() pair.
 
    EXAMPLE
    // Refresh one gadget
    RefreshGList (&gadget, win, NULL, 1);
 
    // Refresh all gadgets in the window
    RefreshGList (win->FirstGadget, win, NULL, -1L);
 
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

    EXTENDWORD(numGad);

    if (!gadgets || !numGad)
        return;

    if ((gadgets->GadgetType & GTYP_REQGADGET) == 0)
    {
        requester = NULL;
    }
    else if (numGad == -2)
    {
        gadgets = requester->ReqGadget;
    }

#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(window);
#endif
 
    int_refreshglist(gadgets,
                     window,
                     requester,
                     numGad,
                     0,
                     0,
                     IntuitionBase);

#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(window);
#endif

    ReturnVoid("RefreshGList");

    AROS_LIBFUNC_EXIT
} /* RefreshGList */

void int_refreshglist(struct Gadget *gadgets, struct Window *window,
                      struct Requester *requester, LONG numGad, LONG mustbe, LONG mustnotbe,
                      struct IntuitionBase *IntuitionBase)
{
#ifdef GADTOOLSCOMPATIBLE

    struct Gadget *gadtoolsgadget = 0;
    LONG           num = numGad;
#endif

    DEBUG_INTREFRESHGLIST(dprintf("IntRefreshGList: Gadgets 0x%lx Window 0x%lx Req 0x%lx Num %ld Must 0x%lx MustNot 0x%lx\n",
                                  gadgets, window, requester, numGad, mustbe, mustnotbe));

    // in case we're not called from RefreshGList...
#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(window);
#endif

    for ( ; gadgets && numGad; gadgets=gadgets->NextGadget, numGad --)
    {
        #ifdef GADTOOLSCOMPATIBLE
        if (gadgets->GadgetType & 0x100)
        {
            gadtoolsgadget = gadgets;
            continue;
        }
        #endif

        if (!(qualifygadget(gadgets,mustbe,mustnotbe,IntuitionBase))) continue;
        //DEBUG_REFRESHGLIST(dprintf("IntRefreshGList: Gadget %p Type 0x%04lx\n",
        //          gadgets, gadgets->GadgetType));

        D(bug("RefreshGList: gadget=%p type 0x%x [%d %d %d %d]\n",
              gadgets,gadgets->GadgetType,
              gadgets->LeftEdge,gadgets->TopEdge,
              gadgets->Width,gadgets->Height));

        /*SetAPen(window->RPort, 4);
        Move(window->RPort, gadgets->LeftEdge-1, gadgets->TopEdge-1);
        Draw(window->RPort, gadgets->LeftEdge+gadgets->Width, gadgets->TopEdge-1);
        Draw(window->RPort, gadgets->LeftEdge+gadgets->Width, gadgets->TopEdge+gadgets->Height);
        Draw(window->RPort, gadgets->LeftEdge-1, gadgets->TopEdge+gadgets->Height);
        Draw(window->RPort, gadgets->LeftEdge-1, gadgets->TopEdge-1);*/

        rendergadget(gadgets,window,requester,IntuitionBase);
    } /* for ( ; gadgets && numGad; gadgets=gadgets->NextGadget, numGad --) */

#ifdef GADTOOLSCOMPATIBLE
    if (gadtoolsgadget)
    {
        for ( ; gadtoolsgadget && num; num --)
        {
            if ((gadtoolsgadget->GadgetType & 0x100) && (qualifygadget(gadtoolsgadget,mustbe,mustnotbe,IntuitionBase))) rendergadget(gadtoolsgadget,window,requester,IntuitionBase);
            gadtoolsgadget = findprevgadget(gadtoolsgadget,window,IntuitionBase);
        }
    }
#endif

#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(window);
#endif
}

BOOL qualifygadget(struct Gadget *gadgets,LONG mustbe, LONG mustnotbe,struct IntuitionBase *IntuitionBase)
{
    if ((mustbe != 0) || (mustnotbe != 0))
    {
        if (gadgets->Activation & (GACT_LEFTBORDER | GACT_RIGHTBORDER |
                                   GACT_TOPBORDER  | GACT_BOTTOMBORDER |
                                   GACT_BORDERSNIFF))
        {
            if (mustnotbe & REFRESHGAD_BORDER) return FALSE; /* don't refresh if border gadget */
        }
        else
        {
            if (mustbe & REFRESHGAD_BORDER) return FALSE; /* don't refresh if not a border gadget */
        }

        if (gadgets->Activation & GACT_TOPBORDER)
        {
            if (mustnotbe & REFRESHGAD_TOPBORDER) return FALSE; /* don't refresh if border gadget */
        }
        else
        {
            if (mustbe & REFRESHGAD_TOPBORDER) return FALSE; /* don't refresh if not a border gadget */
        }

        if (gadgets->Flags & (GFLG_RELRIGHT | GFLG_RELBOTTOM |
                              GFLG_RELWIDTH | GFLG_RELHEIGHT))
        {
            if (mustnotbe & REFRESHGAD_REL) return FALSE; /* don't refresh if rel??? gadget */
        }
        else
        {
            if (mustbe & REFRESHGAD_REL) return FALSE; /* don't refresh if not rel??? gadget */
        }

        if (gadgets->Flags & GFLG_RELSPECIAL)
        {
            if (mustnotbe & REFRESHGAD_RELS) return FALSE; /* don't refresh if relspecial gadget */
        }
        else
        {
            if (mustbe & REFRESHGAD_RELS) return FALSE; /* don't refresh if not relspecial gadget */
        }

        if ((gadgets->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
        {
            if (mustnotbe & REFRESHGAD_BOOPSI) return FALSE; /* don't refresh if boopsi gadget */
        }
        else
        {
            if (mustbe & REFRESHGAD_BOOPSI) return FALSE; /* don't refresh if not boopsi gadget */
        }

    } /* if ((mustbe != 0) || (mustnotbe != 0)) */
    return TRUE;
}

void rendergadget(struct Gadget *gadgets,struct Window *window, struct Requester *requester,struct IntuitionBase *IntuitionBase)
{
    switch (gadgets->GadgetType & GTYP_GTYPEMASK)
    {
	case GTYP_BOOLGADGET:
            RefreshBoolGadget (gadgets, window, requester, IntuitionBase);
            break;

	case GTYP_GADGET0002:
            break;

	case GTYP_PROPGADGET:
            RefreshPropGadget (gadgets, window, requester, IntuitionBase);
            break;

	case GTYP_STRGADGET:
            RefreshStrGadget (gadgets, window, requester, IntuitionBase);
            break;

	case GTYP_CUSTOMGADGET:
            RefreshBoopsiGadget (gadgets, window, requester, IntuitionBase);
            break;

	default:
            RefreshBoolGadget (gadgets, window, requester, IntuitionBase);
            break;

    } /* switch GadgetType */
}

struct Gadget *findprevgadget(struct Gadget *gadget,struct Window *window,struct IntuitionBase *IntuitionBase)
{
    struct Gadget *prevgad = 0, *gad;
    
    for (gad = window->FirstGadget; gad; gad = gad->NextGadget)
    {
        if (gad == gadget) return prevgad;
        prevgad = gad;
    }
    
    return 0;
}

