/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/gadgetclass.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <stdlib.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"

#ifdef __MORPHOS__
#   include "mosmisc.h"
#endif

struct ScrollWindowRasterMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_scrollwindowraster(struct ScrollWindowRasterMsg *msg,
                                   struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH7(void, ScrollWindowRaster,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, win , A1),
         AROS_LHA(WORD           , dx  , D0),
         AROS_LHA(WORD           , dy  , D1),
         AROS_LHA(WORD           , xmin, D2),
         AROS_LHA(WORD           , ymin, D3),
         AROS_LHA(WORD           , xmax, D4),
         AROS_LHA(WORD           , ymax, D5),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 133, Intuition)

/*  FUNCTION
        Scrolls the content of the rectangle defined by (xmin,ymin)-
        (xmax,ymax) by (dx,dy) towards (0,0). This function calls
        ScrollRasterBF().
        The advantage of this function over calling ScrollRasterBF() is
        that the window will be informed about damages. A damage happens
        if in a simple window parts from concelealed areas are scrolled
        to visible areas. The visible areas will be blank as simple
        windows store no data for concealed areas.
        The blank parts that appear due to the scroll will be filled
        with EraseRect() and are not considered damaged areas.

    INPUTS
        win       - pointer to window in which to scroll
        dx,dy     - scroll by (dx,dy) towards (0,0)
        xmin,ymin - upper left corner of the rectangle that will be
                    affected by the scroll
        xmax,ymax - lower rigfht corner of the rectangle that will be
                    affected by the scroll

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_SCROLLWINDOWRASTER(dprintf("ScrollWindowRaster: window 0x%lx dx %d dy %d (%d,%d)-(%d,%d)\n",
                     win, dx, dy, xmin, ymin, xmax, ymax));

    SANITY_CHECK(win)

#ifdef __MORPHOS__
    LockLayers(&win->WScreen->LayerInfo);
#endif

    ScrollRasterBF(win->RPort,
               dx,
               dy,
               xmin,
               ymin,
               xmax,
               ymax);

    /* Has there been damage to the layer? */
    if (WLAYER(win)->Flags & LAYERREFRESH)
    {
        /*
           Send a refresh message to the window if it doesn't already
           have one.
        */

        struct ScrollWindowRasterMsg msg;
        msg.window = win;

    #ifdef DAMAGECACHE
         RecordDamage(win->WScreen,IntuitionBase);
    #endif

    #ifdef __MORPHOS
        UnlockLayers(&win->WScreen->LayerInfo);
    #endif
    
        DoASyncAction((APTR)int_scrollwindowraster, &msg.msg, sizeof(msg), IntuitionBase);
    }
    else
    {
    #ifdef __MORPHOS__
        UnlockLayers(&win->WScreen->LayerInfo);
    #endif
    }

    AROS_LIBFUNC_EXIT
} /* ScrollWindowRaster */

AROS_LH7(void, ScrollWindowRasterNoFill,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, win , A1),
         AROS_LHA(WORD           , dx  , D0),
         AROS_LHA(WORD           , dy  , D1),
         AROS_LHA(WORD           , xmin, D2),
         AROS_LHA(WORD           , ymin, D3),
         AROS_LHA(WORD           , xmax, D4),
         AROS_LHA(WORD           , ymax, D5),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 159, Intuition)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_SCROLLWINDOWRASTER(dprintf("ScrollWindowRaster: window 0x%lx dx %d dy %d (%d,%d)-(%d,%d)\n",
                     win, dx, dy, xmin, ymin, xmax, ymax));

    SANITY_CHECK(win)

#ifdef __MORPHOS__
    LockLayers(&win->WScreen->LayerInfo);
#endif

    {
        WORD adx = abs(dx);
        WORD ady = abs(dy);
        WORD width = xmax - xmin + 1;
        WORD height = ymax - ymin + 1;
        BOOL scroll = TRUE;

        if (adx<width && ady<height)
        {
            WORD cw = width  - adx;
            WORD ch = height - ady;
            WORD x1 = xmin;
            WORD x2 = xmin;
            WORD y1 = ymin;
            WORD y2 = ymin;

            if (dx>=0) x1 += dx;
            else       x2 -= dx;

            if (dy>=0) y1 += dy;
            else       y2 -= dy;

            ClipBlit(win->RPort,x1,y1,win->RPort,x2,y2,cw,ch,0xc0);
        }

        if (!(WLAYER(win)->Flags & LAYERREFRESH))
        {
            struct Rectangle rect;
            struct ClipRect *cr;

            rect.MinX = WLAYER(win)->bounds.MinX + xmin;
            rect.MinY = WLAYER(win)->bounds.MinY + ymin;
            rect.MaxX = WLAYER(win)->bounds.MinX + xmax;
            rect.MaxY = WLAYER(win)->bounds.MinY + ymax;

            for (cr=WLAYER(win)->ClipRect;cr;cr=cr->Next)
            {
                if (rect.MinX < cr->bounds.MinX) continue;
                if (rect.MaxX > cr->bounds.MaxX) continue;
                if (rect.MinY < cr->bounds.MinY) continue;
                if (rect.MaxY > cr->bounds.MaxY) continue;
                scroll = FALSE;
                break;
            }
        }

        if (scroll)
        {
            ULONG mask = win->RPort->Mask;
            win->RPort->Mask = 0;
            ScrollRaster(win->RPort,dx,dy,xmin,ymin,xmax,ymax);
            win->RPort->Mask = mask;
        }
    }

    /* Has there been damage to the layer? */
    if (WLAYER(win)->Flags & LAYERREFRESH)
    {
        /*
           Send a refresh message to the window if it doesn't already
           have one.
        */

        struct ScrollWindowRasterMsg msg;
        msg.window = win;

    #ifdef DAMAGECACHE
        RecordDamage(win->WScreen,IntuitionBase);
    #endif

    #ifdef __MORPHOS__
        UnlockLayers(&win->WScreen->LayerInfo);
    #endif
        DoASyncAction((APTR)int_scrollwindowraster, &msg.msg, sizeof(msg), IntuitionBase);
    }
    else
    {
    #ifdef __MORPHOS__
        UnlockLayers(&win->WScreen->LayerInfo);
    #endif
    }

    AROS_LIBFUNC_EXIT
} /* ScrollWindowRaster */

static VOID int_scrollwindowraster(struct ScrollWindowRasterMsg *msg,
                                   struct IntuitionBase *IntuitionBase)
{
    struct Window *window = msg->window;

    LOCK_REFRESH(window->WScreen);

    if (WLAYER(window)->Flags & LAYERREFRESH)
        WindowNeedsRefresh(window, IntuitionBase);

    UNLOCK_REFRESH(window->WScreen);
}
