/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Responsible for executing deferred Intuition actions like MoveWindow, 
    SizeWindow, ActivateWindow, etc.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/input.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define LOCK_ACTIONS()      ObtainSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);
#define UNLOCK_ACTIONS()    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);

#ifndef __MORPHOS__
static void move_family(struct Window *, int , int);
#endif

/*******************************************************************************************************/

static void CheckLayerRefresh(struct Layer *lay, struct Screen *targetscreen,
                              struct IntuitionBase *IntuitionBase)
{
  //if (lay->Flags & LAYERREFRESH)
    {
        struct Window *win = (struct Window *)lay->Window;

        if (lay == targetscreen->BarLayer)
        {
            RenderScreenBar(targetscreen, TRUE, IntuitionBase);
        }
        else if (win)
        {
            /* Does it belong to a GZZ window and is it
               the outer window of that GZZ window? */
            if (IS_GZZWINDOW(win) && (lay == BLAYER(win)))
            {
                /* simply refresh that window's frame */
                Gad_BeginUpdate(lay, IntuitionBase);
                int_refreshwindowframe(win,REFRESHGAD_BORDER,0,IntuitionBase);
                lay->Flags &= ~LAYERREFRESH;
                Gad_EndUpdate(lay, TRUE, IntuitionBase);
            }
            /* Is it the window layer ? */
            else if (lay == WLAYER(win))
            {
                WindowNeedsRefresh(win, IntuitionBase);
            }
            /* Otherwise, it's a requester. */
            else
            {
                struct Requester *req;

                for (req = win->FirstRequest; req && req->ReqLayer != lay; req = req->OlderRequest);

                if (req)
                {
                    /* FIXME: should send an IDCMP refresh message too. */
                    Gad_BeginUpdate(lay, IntuitionBase);
                    render_requester(req, IntuitionBase);
                    lay->Flags &= ~LAYERREFRESH;
                    Gad_EndUpdate(lay, TRUE, IntuitionBase);
                }
            }
        }

    } /* if (lay->Flags & LAYERREFRESH) */
}

/*******************************************************************************************************/

void CheckLayers(struct Screen *screen, struct IntuitionBase *IntuitionBase)
{
    struct Layer *L;

    LOCK_REFRESH(screen);

    for (L = screen->LayerInfo.top_layer; L; L = L->back)
    {
        if (L->Flags & LAYERREFRESH)
        {
            CheckLayerRefresh(L, screen, IntuitionBase);
        }
    }

    UNLOCK_REFRESH(screen);
}

void WindowSizeWillChange(struct Window *targetwindow, WORD dx, WORD dy,
                                 struct IntuitionBase *IntuitionBase)
{
    struct Rectangle *clipto = NULL;
    struct Rectangle  final_innerrect;
    
    /* Erase the old frame on the right/lower side if
       new size is bigger than old size
    */

    D(bug("********* WindowSizeWillChange ******** dx = %d  dy = %d\n", dx, dy));

    if (AVOID_WINBORDERERASE)
    {
	final_innerrect.MinX = targetwindow->BorderLeft;
	final_innerrect.MinY = targetwindow->BorderTop;
	final_innerrect.MaxX = targetwindow->Width  + dx - 1 - targetwindow->BorderRight;
	final_innerrect.MaxY = targetwindow->Height + dy - 1 - targetwindow->BorderBottom;    
	clipto = &final_innerrect;
    }

    if ( ((dx > 0) && (targetwindow->BorderRight  > 0)) ||
        ((dy > 0) && (targetwindow->BorderBottom > 0)) )
    {
        struct RastPort     *rp = targetwindow->BorderRPort;
        struct Layer        *L = (BLAYER(targetwindow)) ? BLAYER(targetwindow) : WLAYER(targetwindow);
        struct Rectangle     rect;
        struct Region       *oldclipregion;
        WORD                 ScrollX;
        WORD                 ScrollY;

        /*
        ** In case a clip region is installed then I have to
        ** install the regular cliprects of the layer
        ** first. Otherwise the frame might not get cleared correctly.
        */
    	
        LockLayer(0, L);

        oldclipregion = InstallClipRegion(L, NULL);

        ScrollX = L->Scroll_X;
        ScrollY = L->Scroll_Y;

        L->Scroll_X = 0;
        L->Scroll_Y = 0;

        if ((dx > 0) && (targetwindow->BorderRight > 0))
        {
            rect.MinX = targetwindow->Width - targetwindow->BorderRight;
            rect.MinY = 0;
            rect.MaxX = targetwindow->Width - 1;
            rect.MaxY = targetwindow->Height - 1;

    	    OrRectRegion(L->DamageList, &rect);
            L->Flags |= LAYERREFRESH;

    	    if (!AVOID_WINBORDERERASE || AndRectRect(&rect, &final_innerrect, &rect))
	    {
            	EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
	    }

        }

        if ((dy > 0) && (targetwindow->BorderBottom > 0))

        {
            rect.MinX = 0;
            rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
            rect.MaxX = targetwindow->Width - 1;
            rect.MaxY = targetwindow->Height - 1;

	    OrRectRegion(L->DamageList, &rect);
            L->Flags |= LAYERREFRESH;

    	    if (!AVOID_WINBORDERERASE || AndRectRect(&rect, &final_innerrect, &rect))
	    {
            	EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
    	    }
            
        }

        /*
        ** Reinstall the clipregions rectangles if there are any.
        */
        if (NULL != oldclipregion)
        {
            InstallClipRegion(L, oldclipregion);
        }

        L->Scroll_X = ScrollX;
        L->Scroll_Y = ScrollY;

        UnlockLayer(L);

    } /* if ( ((dx > 0) && (targetwindow->BorderRight  > 0)) || ((dy > 0) && (targetwindow->BorderBottom > 0)) ) */

    /* Before resizing the layers eraserect the area of all
       GFLG_REL??? gadgets and add the area to the damagelist */

    EraseRelGadgetArea(targetwindow, clipto, FALSE, IntuitionBase);

}

/*******************************************************************************************************/

void WindowSizeHasChanged(struct Window *targetwindow, WORD dx, WORD dy,
                                 BOOL is_sizewindow, struct IntuitionBase *IntuitionBase)
{
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct Layer    *lay;

    D(bug("********* WindowSizeHasChanged ********\n"));

    lay = (BLAYER(targetwindow)) ? BLAYER(targetwindow) : WLAYER(targetwindow);

    /* Fix GadgetInfo domain if there is an active gadget in window
       which was resized */

    if ((iihdata->ActiveGadget) && (targetwindow == iihdata->GadgetInfo.gi_Window))
    {
        GetGadgetDomain(iihdata->ActiveGadget,
                	iihdata->GadgetInfo.gi_Screen,
                	iihdata->GadgetInfo.gi_Window,
                	NULL,
                	&iihdata->GadgetInfo.gi_Domain);
    }

    /* Relayout GFLG_REL??? gadgets */
    DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

    /* Add the new area of all GFLG_REL??? gadgets to the damagelist, but
       don't EraseRect() as the gadgets will be re-rendered at their new
       position anyway */
       
    {
    	struct Rectangle innerrect;
	
	innerrect.MinX = targetwindow->BorderLeft;
	innerrect.MinY = targetwindow->BorderTop;
	innerrect.MaxX = targetwindow->Width - 1 - targetwindow->BorderRight;
	innerrect.MaxY = targetwindow->Height - 1 - targetwindow->BorderBottom;
	
    	EraseRelGadgetArea(targetwindow, AVOID_WINBORDERERASE ? &innerrect : NULL, TRUE, IntuitionBase);
    }

    /* If new size is smaller than old size add right/bottom
       frame to damagelist */
    if ( ((dx < 0) && (targetwindow->BorderRight  > 0)) ||
        ((dx > 0) && (targetwindow->BorderTop > 0)) ||
        ((dy < 0) && (targetwindow->BorderBottom > 0)) )
    {
        struct Rectangle rect;

        LockLayer(0, lay);

        if ((dx < 0) && (targetwindow->BorderRight > 0))
        {
            rect.MinX = targetwindow->Width - targetwindow->BorderRight;
            rect.MinY = 0;
            rect.MaxX = targetwindow->Width - 1;
            rect.MaxY = targetwindow->Height - 1;

            OrRectRegion(lay->DamageList, &rect);
            lay->Flags |= LAYERREFRESH;
        }

        if ((dx > 0) && (targetwindow->BorderTop > 0))
        {
            rect.MinX = 0;
            rect.MinY = 0;
            rect.MaxX = targetwindow->Width - 1;
            rect.MaxY = targetwindow->BorderTop - 1;

            OrRectRegion(lay->DamageList, &rect);
            lay->Flags |= LAYERREFRESH;
        }

        if ((dy < 0) && (targetwindow->BorderBottom > 0))
        {
            rect.MinX = 0;
            rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
            rect.MaxX = targetwindow->Width - 1;
            rect.MaxY = targetwindow->Height - 1;

            OrRectRegion(lay->DamageList, &rect);
            lay->Flags |= LAYERREFRESH;
        }

        UnlockLayer(lay);

    } /* if ( ((dx < 0) && (targetwindow->BorderRight > 0)) || ((dy < 0) && (targetwindow->BorderBottom > 0)) ) */

    ((struct IntWindow *)(targetwindow))->specialflags |= SPFLAG_LAYERRESIZED;

#if 0
    if (IS_GZZWINDOW(targetwindow))
    {
        lay = targetwindow->BorderRPort->Layer;

        if (lay->Flags & LAYERREFRESH)
        {
            Gad_BeginUpdate(lay, IntuitionBase);
            RefreshWindowFrame(targetwindow);
            lay->Flags &= ~LAYERREFRESH;
            Gad_EndUpdate(lay, TRUE, IntuitionBase);
        }

        lay = targetwindow->WLayer;

        if (lay->Flags & LAYERREFRESH)
        {
            Gad_BeginUpdate(lay, IntuitionBase);
            int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
            Gad_EndUpdate(lay, IS_NOCAREREFRESH(targetwindow), IntuitionBase);
        }

    }
    else
    {
        lay = targetwindow->WLayer;

        if (lay->Flags & LAYERREFRESH)
        {
            Gad_BeginUpdate(lay, IntuitionBase);
            RefreshWindowFrame(targetwindow);
            int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
            Gad_EndUpdate(lay, IS_NOCAREREFRESH(targetwindow), IntuitionBase);
        }
    }

    lay = targetwindow->WLayer;

    if (IS_NOCAREREFRESH(targetwindow))
    {
        LockLayer(0, lay);
        lay->Flags &= ~LAYERREFRESH;
        UnlockLayer(lay);
    }
#endif

#if 0
    //if (is_sizewindow)
    {
        /* Send IDCMP_NEWSIZE to resized window */

        ih_fire_intuimessage(targetwindow,
                     IDCMP_NEWSIZE,
                     0,
                     targetwindow,
                     IntuitionBase);
    }

    if (ie = AllocInputEvent(iihdata))
    {
        ie->ie_Class = IECLASS_EVENT;
        ie->ie_Code = IECODE_NEWSIZE;
        ie->ie_EventAddress = targetwindow;
        CurrentTime(&ie->ie_TimeStamp.tv_secs, &ie->ie_TimeStamp.tv_micro);
    }

    /* Send IDCMP_CHANGEWINDOW to resized window */

    ih_fire_intuimessage(targetwindow,
                 IDCMP_CHANGEWINDOW,
                 CWCODE_MOVESIZE,
                 targetwindow,
                 IntuitionBase);
#endif

}

/*******************************************************************************************************/

void DoMoveSizeWindow(struct Window *targetwindow, LONG NewLeftEdge, LONG NewTopEdge,
                      LONG NewWidth, LONG NewHeight, BOOL send_newsize, struct IntuitionBase *IntuitionBase)
{
    struct IIHData  	*iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
  //struct IntWindow  	*w       = (struct IntWindow *)targetwindow;
    struct Layer    	*targetlayer = WLAYER(targetwindow)/*, *L*/;
    struct Requester    *req;
    struct InputEvent   *ie;
    LONG            	 OldLeftEdge  = targetwindow->LeftEdge;
    LONG            	 OldTopEdge   = targetwindow->TopEdge;
    LONG            	 OldWidth     = targetwindow->Width;
    LONG            	 OldHeight    = targetwindow->Height;
    LONG            	 pos_dx, pos_dy, size_dx, size_dy;

    /* correct new window coords if necessary */

    FixWindowCoords(targetwindow, &NewLeftEdge, &NewTopEdge, &NewWidth, &NewHeight,IntuitionBase);

    D(bug("DoMoveSizeWindow to %d,%d %d x %d\n", NewLeftEdge, NewTopEdge, NewWidth, NewHeight));

    pos_dx  = NewLeftEdge - OldLeftEdge;
    pos_dy  = NewTopEdge  - OldTopEdge;
    size_dx = NewWidth    - OldWidth;
    size_dy = NewHeight   - OldHeight;

    LOCK_REFRESH(targetwindow->WScreen);

/* jDc: intuition 68k doesn't care about that */
//    if (pos_dx || pos_dy || size_dx || size_dy)
//    {

        if (size_dx || size_dy)
        {
            WindowSizeWillChange(targetwindow, size_dx, size_dy, IntuitionBase);
        }

        targetwindow->LeftEdge    += pos_dx;
        targetwindow->TopEdge     += pos_dy;
#ifndef __MORPHOS__
        targetwindow->RelLeftEdge += pos_dx;
        targetwindow->RelTopEdge  += pos_dy;
#endif

        targetwindow->Width     = NewWidth;
        targetwindow->Height    = NewHeight;
        targetwindow->GZZWidth  = targetwindow->Width  - targetwindow->BorderLeft - targetwindow->BorderRight;
        targetwindow->GZZHeight = targetwindow->Height - targetwindow->BorderTop  - targetwindow->BorderBottom;

        /* check for GZZ window */
        if (BLAYER(targetwindow))
        {
            /* move outer window first */
            MoveSizeLayer(BLAYER(targetwindow), pos_dx, pos_dy, size_dx, size_dy);
        }

        MoveSizeLayer(targetlayer, pos_dx, pos_dy, size_dx, size_dy);

        for (req = targetwindow->FirstRequest; req; req = req->OlderRequest)
        {
            struct Layer *layer = req->ReqLayer;

            if (layer)
            {
                int dx, dy, dw, dh;
                int left, top, right, bottom;

                left = NewLeftEdge + req->LeftEdge;
                top = NewTopEdge + req->TopEdge;
                right = left + req->Width - 1;
                bottom = top + req->Height - 1;

                if (left > NewLeftEdge + NewWidth - 1)
                    left = NewLeftEdge + NewWidth - 1;

                if (top > NewTopEdge + NewHeight - 1)
                    top = NewTopEdge + NewHeight - 1;

                if (right > NewLeftEdge + NewWidth - 1)
                    right = NewLeftEdge + NewWidth - 1;

                if (bottom > NewTopEdge + NewHeight - 1)
                    bottom = NewTopEdge + NewHeight - 1;

                dx = left - layer->bounds.MinX;
                dy = top - layer->bounds.MinY;
                dw = right - left - layer->bounds.MaxX + layer->bounds.MinX;
                dh = bottom - top - layer->bounds.MaxY + layer->bounds.MinY;

                MoveSizeLayer(layer, dx, dy, dw, dh);
            }
        }

#if 0
        if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = OldLeftEdge;
        if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = OldTopEdge;
        if (w->ZipWidth    != ~0) w->ZipWidth    = OldWidth;
        if (w->ZipHeight   != ~0) w->ZipHeight   = OldHeight;
#endif

        if (pos_dx || pos_dy)
        {
            UpdateMouseCoords(targetwindow);
#ifndef __MORPHOS__
            if (HAS_CHILDREN(targetwindow))
                move_family(targetwindow, pos_dx, pos_dy);
#endif
        }

//    } /* if (pos_dx || pos_dy || size_dx || size_dy) */

    if (size_dx || size_dy)
    {
        WindowSizeHasChanged(targetwindow, size_dx, size_dy, FALSE, IntuitionBase);
    }

    ih_fire_intuimessage(targetwindow,
                 IDCMP_CHANGEWINDOW,
                 CWCODE_MOVESIZE,
                 targetwindow,
                 IntuitionBase);

    if (send_newsize)
    {
        /* Send IDCMP_NEWSIZE and IDCMP_CHANGEWINDOW to resized window, even
           if there was no resizing/position change at all. BGUI for example
           relies on this! */

        ih_fire_intuimessage(targetwindow,
                     IDCMP_NEWSIZE,
                     0,
                     targetwindow,
                     IntuitionBase);

        if ((ie = AllocInputEvent(iihdata)))
        {
            ie->ie_Class = IECLASS_EVENT;
            ie->ie_Code = IECODE_NEWSIZE;
            ie->ie_EventAddress = targetwindow;
            CurrentTime(&ie->ie_TimeStamp.tv_secs, &ie->ie_TimeStamp.tv_micro);
        }
    }

    // jDc: CheckLayers calls LOCK_REFRESH, so there's no reason to UNLOCK here!
    //    UNLOCK_REFRESH(targetwindow->WScreen);

    CheckLayers(targetwindow->WScreen, IntuitionBase);

    UNLOCK_REFRESH(targetwindow->WScreen);


}

/*******************************************************************************************************/

void DoSyncAction(void (*func)(struct IntuiActionMsg *, struct IntuitionBase *),
                  struct IntuiActionMsg *msg,
                  struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct Task    *me = FindTask(NULL);

    if (me == iihd->InputDeviceTask)
    {
        func(msg, IntuitionBase);
    }
    else
    {
    #ifdef __MORPHOS__
        struct IOStdReq   req;
        struct MsgPort    port;
        struct InputEvent ie;
    #endif

        msg->handler = func;
        msg->task    = me;
        msg->done    = FALSE;
    
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);
        AddTail((struct List *)GetPrivIBase(IntuitionBase)->IntuiActionQueue, (struct Node *)msg);
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);

    #ifdef __MORPHOS__
        port.mp_Flags 	= PA_SIGNAL;
        port.mp_SigTask = me;
        port.mp_SigBit  = SIGB_INTUITION;
        NEWLIST(&port.mp_MsgList);

        req.io_Message.mn_ReplyPort = &port;
        req.io_Device 	    	    = GetPrivIBase(IntuitionBase)->InputIO->io_Device;
        req.io_Unit 	    	    = GetPrivIBase(IntuitionBase)->InputIO->io_Unit;
        req.io_Command      	    = IND_WRITEEVENT;
        req.io_Length 	    	    = sizeof(ie);
        req.io_Data 	    	    = &ie;

        ie.ie_Class = IECLASS_NULL;
    #endif
    
        if (!msg->done)
        {
    	#ifdef __MORPHOS__
            DoIO((APTR)&req);
    	#else
    	    AddNullEvent();
    	#endif
            while (!msg->done)
            {
                Wait(SIGF_INTUITION);
            }
        }
    }
}

/*******************************************************************************************************/

BOOL DoASyncAction(void (*func)(struct IntuiActionMsg *, struct IntuitionBase *),
                   struct IntuiActionMsg *msg, ULONG size,
                   struct IntuitionBase *IntuitionBase)
{
    struct IIHData  	    *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct Task     	    *me = FindTask(NULL);
    struct IntuiActionMsg   *new_msg;

    if (me == iihd->InputDeviceTask)
    {
        func(msg, IntuitionBase);
        return TRUE;
    }
    else if ((new_msg = AllocVecPooled(iihd->ActionsMemPool,size)))
    {
        new_msg->handler = func;
        new_msg->task = NULL;
        if (size > sizeof(*msg))
        {
            CopyMem(msg + 1, new_msg + 1, size - sizeof(*msg));
        }

        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);
        AddTail((struct List *)GetPrivIBase(IntuitionBase)->IntuiActionQueue, (struct Node *)new_msg);
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);

    #ifndef __MORPHOS__
    	AddNullEvent();
    #endif
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************************************/

void HandleIntuiActions(struct IIHData *iihdata,
                        struct IntuitionBase *IntuitionBase)
{
    struct IntuiActionMsg *am;

    D(bug("Handle Intuition action messages\n"));

    if (iihdata->ActiveSysGadget)
    {
     	D(bug("Handle Intuition action messages. Doing nothing because of active drag or resize gadget!\n"));   	
	return;
    }
    
    for (;;)
    {
        LOCK_ACTIONS();
        am = (struct IntuiActionMsg *)RemHead((struct List *)&iihdata->IntuiActionQueue);
        UNLOCK_ACTIONS();

        if (!am)
            break;

        am->handler(am, IntuitionBase);

        if (am->task)
        {
            Forbid();
            am->done = TRUE;
            Signal(am->task, SIGF_INTUITION);
            Permit();
        }
        else
        {
            FreeVecPooled(iihdata->ActionsMemPool,am);
        }
    }

    D(bug("Intuition action messages handled\n"));
}

#ifndef __MORPHOS__
static void move_family(struct Window * w, int dx, int dy)
{
    struct Window * _w = w->firstchild;

    while (_w)
    {
        _w->LeftEdge  += dx,
                 _w->TopEdge   += dy;
        if (HAS_CHILDREN(_w))
            move_family(_w,dx,dy);
        _w=_w->nextchild;
    }
}
#endif
