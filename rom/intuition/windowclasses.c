/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
    
    Classes for window decor stuff, like dragbar, close etc.
*/

/***********************************************************************************/

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/layers.h>

#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/extensions.h>
#include <aros/asmcall.h>

#include "gadgets.h"

#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"

#ifdef SKINS
#include "intuition_customizesupport.h"
#include "renderwindowframe.h"
#include "mosmisc.h"
#endif

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define MOVEHACK (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_PRIVILEDGEDREFRESH)

/*

jDc: opaque resize code is NOT sutiable for other apps than MUI, I have no intentions in fixing it so that
it would work with all apps out there. Use mui or die :)

The idea behind the trick is that we use a func that checks if apps reply to idcmps in some period
of time. As long as they are replying fast we can do another layer resize, send idcmp to app.
If they do not then we wait for a while and after some time, if one or more apps don't reply
we do the resize again (the timeout is also useful when we have some dead app with windows
on screen.

*/

#if USE_OPAQUESIZE
#   define OPAQUESIZE (w->MoreFlags & WMFLG_IAMMUI)
#else
#   define OPAQUESIZE 0
#endif

//#define DELAYEDDRAG
//#define DELAYEDSIZE

/***********************************************************************************/

#define WSIG_MOVE SIGF_INTUITION
#define WSIG_DIE  SIGF_ABORT

/***********************************************************************************/

void MoveTask(struct dragbar_data *data,struct Window *w,struct Screen *screen,struct IntuitionBase *IntuitionBase)
{
    ULONG signals;

    for (;;)
    {
        signals = Wait(WSIG_DIE|WSIG_MOVE);

        if (signals & WSIG_DIE)
        {
            Forbid();
            data->movetask = 0;
            Permit();
            WindowAction(w,WAC_CHANGEWINDOWBOX,0);
            return;
        }

        if (signals & WSIG_MOVE)
        {
            struct Layer *L;

            LockLayers(&screen->LayerInfo);

            for (L = screen->LayerInfo.top_layer; L; L = L->back)
            {
                if (L->Window == w)
                {
                    //window not closed yet!
                    if (data->curleft != w->LeftEdge || data->curtop != w->TopEdge)
                    {
                        struct Requester    *req;

                        if (BLAYER(w))
                        {
                            /* move outer window first */
                            MoveSizeLayer(BLAYER(w), data->curleft - w->LeftEdge, data->curtop - w->TopEdge, 0, 0);
                        }

                        MoveSizeLayer(WLAYER(w), data->curleft - w->LeftEdge, data->curtop - w->TopEdge, 0, 0);

                        for (req = w->FirstRequest; req; req = req->OlderRequest)
                        {
                            struct Layer *layer = req->ReqLayer;

                            if (layer)
                            {
                                int dx, dy, dw, dh;
                                int left, top, right, bottom;

                                left = data->curleft + req->LeftEdge;
                                top = data->curtop + req->TopEdge;
                                right = left + req->Width - 1;
                                bottom = top + req->Height - 1;

                                if (left > data->curleft + w->Width - 1)
                                    left = data->curleft + w->Width - 1;

                                if (top > data->curtop + w->Height - 1)
                                    top = data->curtop + w->Height - 1;

                                if (right > data->curleft + w->Width - 1)
                                    right = data->curleft + w->Width - 1;

                                if (bottom > data->curtop + w->Height - 1)
                                    bottom = data->curtop + w->Height - 1;

                                dx = left - layer->bounds.MinX;
                                dy = top - layer->bounds.MinY;
                                dw = right - left - layer->bounds.MaxX + layer->bounds.MinX;
                                dh = bottom - top - layer->bounds.MaxY + layer->bounds.MinY;

                                MoveSizeLayer(layer, dx, dy, dw, dh);
                            }
                        }

                        w->LeftEdge = data->curleft;
                        w->TopEdge = data->curtop;
                        UpdateMouseCoords(w);

                    }
                }
            }

            #ifdef DAMAGECACHE
            RecordDamage(screen,IntuitionBase);
            #endif
            data->drag_refreshed = FALSE;

            UnlockLayers(&screen->LayerInfo);
        }
    }
}

/***********************************************************************************/

/* drawwindowframe is used when the user drags or resizes a window */

#define DWF_THICK_X 2
#define DWF_THICK_Y 2

/***********************************************************************************/

static void cliprectfill(struct Screen *scr, struct RastPort *rp,
                         WORD x1, WORD y1, WORD x2, WORD y2,
                         struct IntuitionBase *IntuitionBase)
{
    WORD scrx2 = scr->Width  - 1;
    WORD scry2 = scr->Height - 1;

    /* Check if inside at all */

    if (!((x1 > scrx2) || (x2 < 0) || (y1 > scry2) || (y2 < 0)))
    {
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 > scrx2) x2 = scrx2;
        if (y2 > scry2) y2 = scry2;

        /* paranoia */

        if ((x2 >= x1) && (y2 >= y1))
        {
            RectFill(rp, x1, y1, x2, y2);
        }
    }

}

/***********************************************************************************/

static void drawwindowframe(struct Screen *scr, struct RastPort *rp,
                            WORD x1, WORD y1, WORD x2, WORD y2,
                            struct IntuitionBase *IntuitionBase)
{
    /* this checks should not be necessary, but just to be sure */

    if (x2 < x1)
    {
        /* swap x2 and x1 */
        x2 ^= x1;
        x1 ^= x2;
        x2 ^= x1;
    }

    if (y2 < y1)
    {
        /* swap y2 and y1 */
        y2 ^= y1;
        y1 ^= y2;
        y2 ^= y1;
    }

    if (((x2 - x1) < (DWF_THICK_X * 2)) ||
        ((y2 - y1) < (DWF_THICK_Y * 2)))
    {
        cliprectfill(scr, rp, x1, y1, x2, y2, IntuitionBase);
    }
    else
    {
        cliprectfill(scr, rp, x1, y1, x2, y1 + DWF_THICK_Y - 1, IntuitionBase);
        cliprectfill(scr, rp, x2 - DWF_THICK_X + 1, y1 + DWF_THICK_Y, x2, y2, IntuitionBase);
        cliprectfill(scr, rp, x1, y2 - DWF_THICK_Y + 1, x2 - DWF_THICK_X, y2, IntuitionBase);
        cliprectfill(scr, rp, x1, y1 + DWF_THICK_Y, x1 + DWF_THICK_X - 1, y2 - DWF_THICK_Y, IntuitionBase);
    }
}

/***********************************************************************************/

#define IntuitionBase ((struct IntuitionBase *)(cl)->cl_UserData)

/***********************************************************************************/

#if 0
IPTR DragBarClass__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender * msg)
{
    EnterFunc(bug("DragBar::Render()\n"));
    /* We will let the AROS gadgetclass test if it is safe to render */

    if ( DoSuperMethodA(cl, (Object *)g, (Msg)msg) != 0)
    {
        struct DrawInfo     *dri = msg->gpr_GInfo->gi_DrInfo;
        UWORD               *pens = dri->dri_Pens;
        struct RastPort     *rp = msg->gpr_RPort;
        struct IBox          container;
        struct Window       *win = msg->gpr_GInfo->gi_Window;
        struct TextExtent    te;

        GetGadgetIBox(g, msg->gpr_GInfo, &container);

        if (container.Width <= 1 || container.Height <= 1)
            return;


        /* Clear the dragbar */

        SetAPen(rp, (win->Flags & WFLG_WINDOWACTIVE) ?
                pens[FILLPEN] : pens[BACKGROUNDPEN]);

        SetDrMd(rp, JAM1);

        D(bug("Filling from (%d, %d) to (%d, %d)\n",
              container.Left,
              container.Top,
              container.Left + container.Width - 1,
              container.Top + container.Height - 1));

        RectFill(rp,
        	 container.Left,
        	 container.Top,
        	 container.Left + container.Width - 1,
        	 container.Top + container.Height - 2);

        /* Draw a thin dark line around the bar */

        SetAPen(rp, pens[SHINEPEN]);
        RectFill(rp,container.Left,
        	 container.Top,
        	 container.Left,
        	 container.Top + container.Height - 1 - ((container.Left == 0) ? 0 : 1));
        RectFill(rp,container.Left + 1,
        	 container.Top,
        	 container.Left + container.Width - 1,
        	 container.Top);

        SetAPen(rp,pens[SHADOWPEN]);
        RectFill(rp,container.Left + container.Width - 1,
        	 container.Top + 1,
        	 container.Left + container.Width - 1,
        	 container.Top + container.Height - 1);
        RectFill(rp,container.Left + ((container.Left == 0) ? 1 : 0),
        	 container.Top + container.Height - 1,
        	 container.Left + container.Width - 2,
        	 container.Top + container.Height - 1);

        /* Render the titlebar */
        if (NULL != win->Title)
        {
            ULONG textlen, titlelen;

            SetFont(rp, dri->dri_Font);

            titlelen = strlen(win->Title);
            textlen = TextFit(rp
                      , win->Title
                      , titlelen
                      , &te
                      , NULL
                      , 1
                      , container.Width - 6
                      , container.Height);

            SetAPen(rp, pens[(win->Flags & WFLG_WINDOWACTIVE) ? FILLTEXTPEN : TEXTPEN]);
            Move(rp, container.Left + 3, container.Top + dri->dri_Font->tf_Baseline + 2);

            Text(rp, win->Title, textlen);
        }

    }  /* if (allowed to render) */

    return (IPTR)0;
}
#endif

/***********************************************************************************/

IPTR DragBarClass__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct InputEvent *ie = msg->gpi_IEvent;
    IPTR    	       retval = GMR_NOREUSE;

    if (ie)
    {
        /* The gadget was activated via mouse input */
        struct dragbar_data *data;
        struct Window       *w;

        /* There is no point in rerendering ourseleves her, as this
           is done by a call to RefreshWindowFrame() in the intuition inputhandler
        */

        w = msg->gpi_GInfo->gi_Window;

        data = INST_DATA(cl, g);

    #ifdef USEWINDOWLOCK
        /* do NOT ObtainSemaphore here since this would lead to deadlocks!!! */
        /* when the semaphore can not be obtained we simply ignore gadget activation */

        /* in movehack the task that locks windowlock is the one that calls MoveWindow*/
        if (!(AttemptSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock)))
        {
            goto fail;
        }
        data->drag_windowlock = TRUE;
    #endif

        data->curleft = w->LeftEdge;
        data->curtop  = w->TopEdge;
        
        data->startleft = w->LeftEdge;
        data->starttop = w->TopEdge;

        data->mousex = w->WScreen->MouseX - data->curleft;
        data->mousey = w->WScreen->MouseY - data->curtop;

        data->drag_refreshed = TRUE;

        data->rp = CloneRastPort(&w->WScreen->RastPort);
        if (data->rp)
        {

            /* Lock all layers while the window is dragged.
             * Get the gadget lock first to avoid deadlocks
             * with ObtainGIRPort. */

            D(bug("locking all layers\n"));

            if (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE))
            {
    	    #ifdef USEGADGETLOCK
                if (AttemptSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock))
                {
                    data->drag_gadgetlock = TRUE;
                }
                else
                {
                    goto fail;
                }
    	    #endif
    	    #ifndef DELAYEDDRAG
                LockLayers(&w->WScreen->LayerInfo);
                data->drag_layerlock = TRUE;
    	    #endif
            } else {
                if (MOVEHACK)
                {
    	    	#ifdef __MORPHOS__
                    data->movetask =
                        NewCreateTask(TASKTAG_CODETYPE, CODETYPE_PPC, TASKTAG_PC, (ULONG)MoveTask,
                                        TASKTAG_PRI, 0,
                                        TASKTAG_PPC_ARG1,(ULONG)data,
                                        TASKTAG_PPC_ARG2,(ULONG)w,
                                        TASKTAG_PPC_ARG3,(ULONG)w->WScreen,
                                        TASKTAG_PPC_ARG4,(ULONG)IntuitionBase,
                                        TAG_DONE);
    	    	#else
    	    	// FIXME!
    	    	#warning Implemente MOVEHACK support for AROS (?)
    	    	#endif
                }
            }

            SetDrMd(data->rp, COMPLEMENT);

    	#ifndef DELAYEDDRAG
            if (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE))
                drawwindowframe(w->WScreen
                        , data->rp
                        , data->curleft
                        , data->curtop
                        , data->curleft + w->Width  - 1
                        , data->curtop  + w->Height - 1
                        , IntuitionBase
                           );

            data->isrendered = TRUE;
    	#endif

            data->drag_canceled = FALSE;

            {
                UQUAD currenttime;
		
                currenttime = ie->ie_TimeStamp.tv_secs;
                currenttime = currenttime * 50;
                currenttime += ie->ie_TimeStamp.tv_micro / 20000;
		
                data->lasteventtime = currenttime;
            }

            /* size mouse bounds such that mouse pointer cannot move if window cannot move, if offscreenlayers is turned off */
	    if (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS)) {
            	struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
		iihd->MouseBoundsActiveFlag = TRUE;
            	iihd->MouseBoundsLeft = data->mousex;
            	iihd->MouseBoundsRight = w->WScreen->Width - (w->Width - data->mousex);
            	iihd->MouseBoundsTop = data->mousey;
            	iihd->MouseBoundsBottom = w->WScreen->Height - (w->Height - data->mousey);
	    }

            return GMR_MEACTIVE;
        }

fail:
        if (data->drag_layerlock)
        {
            UnlockLayers(&w->WScreen->LayerInfo);
            data->drag_layerlock = FALSE;
            ;
        }

    #ifdef USEGADGETLOCK
        if (data->drag_gadgetlock)
        {
            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
            data->drag_gadgetlock = FALSE;
        }
    #endif

    #ifdef USEWINDOWLOCK
        if (data->drag_windowlock)
        {
            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
            data->drag_windowlock = FALSE;
        }
    #endif
    }

    return retval;

}

/***********************************************************************************/

IPTR DragBarClass__GM_MOVETEST(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    IPTR    	    	 retval = MOVETEST_MOVE;
#ifdef SKINS
    struct dragbar_data *data = INST_DATA(cl, g);
    struct Window       *w = msg->gpi_GInfo->gi_Window;
    struct InputEvent 	 myie;
    LONG            	 new_left;
    LONG            	 new_top;

    CopyMem(msg->gpi_IEvent,&myie,sizeof (struct InputEvent));
    myie.ie_Code = 0x68; //mouse_leftpress

    /* Can we move to the new position, or is window at edge of display ? */
    new_left = msg->gpi_Mouse.X - data->mousex;
    new_top  = msg->gpi_Mouse.Y - data->mousey;

    if ((!((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS) && (w->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))) ||
        MatchHotkey(&myie,IA_TOGGLEOFFSCREEN,IntuitionBase))
    {
        if (new_left < 0)
        {
            msg->gpi_Mouse.X -= new_left;
            retval = MOVETEST_ADJUSTPOS;
        }

        if (new_top < 0)
        {
            msg->gpi_Mouse.Y -= new_top;
            retval = MOVETEST_ADJUSTPOS;
        }

        if (new_left + w->Width > w->WScreen->Width)
        {
            msg->gpi_Mouse.X -= new_left - (w->WScreen->Width - w->Width);
            retval = MOVETEST_ADJUSTPOS;
        }

        if (new_top + w->Height > w->WScreen->Height)
        {
            msg->gpi_Mouse.Y -= new_top - (w->WScreen->Height - w->Height);
            retval =  MOVETEST_ADJUSTPOS;
        }
    }
#endif

    return retval;

}

/***********************************************************************************/

IPTR DragBarClass__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct GadgetInfo *gi = msg->gpi_GInfo;
    IPTR    	       retval = GMR_MEACTIVE;

    if (gi)
    {
        struct InputEvent   *ie = msg->gpi_IEvent;
        struct dragbar_data *data = INST_DATA(cl, g);
        struct Window       *w = msg->gpi_GInfo->gi_Window;

        switch (ie->ie_Class)
        {
        case IECLASS_RAWMOUSE:
            switch (ie->ie_Code)
            {
            case MENUDOWN:
                retval = GMR_NOREUSE;
                data->drag_canceled = TRUE;
                break;

            case SELECTUP:
                retval = GMR_NOREUSE;
                break;


            case IECODE_NOBUTTON:
                {
                    struct Screen   	*scr = w->WScreen;
                    struct InputEvent 	 myie;
                    LONG            	 new_left;
                    LONG            	 new_top;

                    /* Can we move to the new position, or is window at edge of display ? */
                    new_left = scr->MouseX - data->mousex;
                    new_top  = scr->MouseY - data->mousey;

                    CopyMem(ie,&myie,sizeof (struct InputEvent));
                    myie.ie_Code = SELECTDOWN; //mouse_leftpress

    	    	#ifdef SKINS
                    if ((!((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS) && (w->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))) ||
                        MatchHotkey(&myie,IA_TOGGLEOFFSCREEN,IntuitionBase))
    	    	#else
                    if (!((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS) && (w->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS)))
    	    	#endif
                    {
                        if (new_left < 0)
                        {
                            data->mousex += new_left;
                            new_left = 0;
                        }

                        if (new_top < 0)
                        {
                            data->mousey += new_top;
                            new_top = 0;
                        }

                        if (new_left + w->Width > scr->Width)
                        {
                            LONG correct_left;
                            correct_left = scr->Width - w->Width; /* align to screen border */
                            data->mousex += new_left - correct_left;
                            new_left = correct_left;
                        }
			
                        if (new_top + w->Height > scr->Height)
                        {
                            LONG correct_top;
                            correct_top = scr->Height - w->Height; /* align to screen border */
                            data->mousey += new_top - correct_top;
                            new_top = correct_top;
                        }
                    }

                    if (data->curleft != new_left || data->curtop != new_top)
                    {
                        SetDrMd(data->rp, COMPLEMENT);

                        if ((data->isrendered) && (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)))
                        {
                            /* Erase old frame */
                            drawwindowframe(w->WScreen
                                	    , data->rp
                                	    , data->curleft
                                	    , data->curtop
                                	    , data->curleft + w->Width  - 1
                                	    , data->curtop  + w->Height - 1
                                	    , IntuitionBase
                                	    );
                        }

                        data->curleft = new_left;
                        data->curtop  = new_top;

                        if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)
                        {
                            WORD newx = new_left - w->LeftEdge, newy = new_top - w->TopEdge;

                            if (newx || newy)
                            {
                                UQUAD currenttime;
				
                                currenttime = ie->ie_TimeStamp.tv_secs;
                                currenttime = currenttime * 50;
                                currenttime += ie->ie_TimeStamp.tv_micro / 20000;

                                if (currenttime > data->lasteventtime + 10) //10 delay should result in intuitick freq
                                {
                                    ih_fire_intuimessage(w,
                                        		 IDCMP_CHANGEWINDOW,
                                        		 CWCODE_MOVESIZE,
                                        		 w,
                                        		 IntuitionBase);
                                    data->lasteventtime = currenttime;
                                }

                                if (data->movetask)
                                {
                                    Forbid();
                                    Signal(data->movetask,WSIG_MOVE);
                                    Permit();
				    
                                    CheckLayers(w->WScreen, IntuitionBase);
                                }
				else
				{
                                    struct Requester *req;

                                    LockLayers(&w->WScreen->LayerInfo);

                                    w->LeftEdge    += newx;
                                    w->TopEdge     += newy;

                                    if (BLAYER(w))
                                    {
                                        MoveSizeLayer(BLAYER(w), newx, newy , 0, 0);
                                    }

                                    MoveSizeLayer(WLAYER(w), newx, newy, 0, 0);

                                    for (req = w->FirstRequest; req; req = req->OlderRequest)
                                    {
                                        struct Layer *layer = req->ReqLayer;

                                        if (layer)
                                        {
                                            int dx, dy, dw, dh;
                                            int left, top, right, bottom;

                                            left = data->curleft + req->LeftEdge;
                                            top = data->curtop + req->TopEdge;
                                            right = left + req->Width - 1;
                                            bottom = top + req->Height - 1;

                                            if (left > data->curleft + w->Width - 1)
                                                left = data->curleft + w->Width - 1;

                                            if (top > data->curtop + w->Height - 1)
                                                top = data->curtop + w->Height - 1;

                                            if (right > data->curleft + w->Width - 1)
                                                right = data->curleft + w->Width - 1;

                                            if (bottom > data->curtop + w->Height - 1)
                                                bottom = data->curtop + w->Height - 1;

                                            dx = left - layer->bounds.MinX;
                                            dy = top - layer->bounds.MinY;
                                            dw = right - left - layer->bounds.MaxX + layer->bounds.MinX;
                                            dh = bottom - top - layer->bounds.MaxY + layer->bounds.MinY;

                                            MoveSizeLayer(layer, dx, dy, dw, dh);
                                        }
                                    }
                                    
                                    CheckLayers(w->WScreen, IntuitionBase);

                                    UnlockLayers(&w->WScreen->LayerInfo);
                                }
                            }
                        }
                        else
                        {
                            #ifdef DELAYEDDRAG
                            if ((!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)) && (!data->drag_layerlock))
                            {
                                LockLayers(&w->WScreen->LayerInfo);
                                data->drag_layerlock = TRUE;
                            }
                            #endif
                                /* Rerender the window frame */
                            drawwindowframe(w->WScreen
                                	    , data->rp
                                	    , data->curleft
                                	    , data->curtop
                                	    , data->curleft + w->Width  - 1
                                	    , data->curtop  + w->Height - 1
                                	    , IntuitionBase);
                            data->isrendered = TRUE;
                        }

                    }

                    retval = GMR_MEACTIVE;

                    break;
                }

            default:
                retval = GMR_NOREUSE;
                if (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)) data->drag_canceled = TRUE;
                break;

            } /* switch (ie->ie_Code) */
            break;

    case IECLASS_TIMER:
            #ifdef DELAYEDDRAG
            if ((!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)) && (!data->drag_layerlock))
            {
                UQUAD currenttime;
		
                currenttime = ie->ie_TimeStamp.tv_secs;
                currenttime = currenttime * 50;
                currenttime += ie->ie_TimeStamp.tv_micro / 20000;

                if (currenttime > data->lasteventtime + 10)
                {
                    LockLayers(&w->WScreen->LayerInfo);
                    data->drag_layerlock = TRUE;

                    drawwindowframe(w->WScreen
                        , data->rp
                        , data->curleft
                        , data->curtop
                        , data->curleft + w->Width  - 1
                        , data->curtop  + w->Height - 1
                        , IntuitionBase
                        );
                    data->lasteventtime = currenttime;
                    data->isrendered = TRUE;
                }
            }
            #endif

            if (data->movetask)
            {
                UQUAD currenttime;
		
                currenttime = ie->ie_TimeStamp.tv_secs;
                currenttime = currenttime * 50;
                currenttime += ie->ie_TimeStamp.tv_micro / 20000;

                if ((!data->drag_refreshed) && currenttime > data->lasteventtime + 10) //10 delay should result in intuitick freq
                {
                    ih_fire_intuimessage(   w,
                          IDCMP_CHANGEWINDOW,
                          CWCODE_MOVESIZE,
                          w,
                          IntuitionBase);
                    data->drag_refreshed = TRUE;
                    data->lasteventtime = currenttime;
                }
                CheckLayers(w->WScreen,IntuitionBase);
            }
            break;
        } /* switch (ie->ie_Class) */

    } /* if (gi) */

    return retval;
}

/***********************************************************************************/

IPTR DragBarClass__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct dragbar_data *data;
    struct Window   	*w;

    data = INST_DATA(cl, g);
    
    w = msg->gpgi_GInfo->gi_Window;

    /* Always clear last drawn frame */

    if (data->isrendered && data->rp && (!(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE)))
    {

        SetDrMd(data->rp, COMPLEMENT);

        /* Erase old frame */
        drawwindowframe(w->WScreen
                , data->rp
                , data->curleft
                , data->curtop
                , data->curleft + w->Width  - 1
                , data->curtop  + w->Height - 1
                , IntuitionBase
                   );

    }

    data->isrendered = FALSE;

    if (!data->drag_refreshed) CheckLayers(w->WScreen, IntuitionBase);

    Forbid();
    if (data->movetask)
    {
        Signal(data->movetask,WSIG_DIE);
        data->movetask = 0;
    }
    Permit();

    if (!data->drag_canceled)// && !(GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE))
    {
        MoveWindow(w
        	   , data->curleft - w->LeftEdge    /* dx */
        	   , data->curtop  - w->TopEdge /* dy */
        	  );

    }

    if (data->drag_canceled && (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OPAQUEMOVE))
    {
        MoveWindow(w
        	   , data->startleft - w->LeftEdge /* dx */
        	   , data->starttop - w->TopEdge /* dy */
        	  );
    }


    ih_fire_intuimessage(w,
        		 IDCMP_CHANGEWINDOW,
        		 CWCODE_MOVESIZE,
        		 w,
        		 IntuitionBase);

    data->drag_canceled = TRUE;

    if (data->drag_layerlock)
    {
        UnlockLayers(&w->WScreen->LayerInfo);
        data->drag_layerlock = FALSE;
    }

#ifdef USEGADGETLOCK
    if (data->drag_gadgetlock)
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
        data->drag_gadgetlock = FALSE;
    }
#endif

#ifdef USEWINDOWLOCK
    if (data->drag_windowlock)
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
        data->drag_windowlock = FALSE;
    }
#endif

    /* User throught with drag operation. Unlock layesr and free
    rastport clone
    */

    if (data->rp)
    {
        FreeRastPort(data->rp);
        data->rp = NULL;
    }

    /* shut off mouse bounds checking.  */
    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    iihd->MouseBoundsActiveFlag = FALSE;

    return TRUE;


}

/***********************************************************************************/

IPTR DragBarClass__NOP(Class *cl, Object *o, Msg msg)
{
    return (IPTR)0;
}

/***********************************************************************************/

IPTR DragBarClass__GM_HITTEST(Class *cl, Object *o, Msg msg)
{
    return (IPTR)1;
}

/***********************************************************************************/

IPTR DragBarClass__OM_NEW(Class *cl, Object *o, Msg msg)
{
    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, msg);
    if (g)
    {
	g->GadgetType |= GTYP_SYSGADGET | GTYP_WDRAGGING;
    }
    
    return (IPTR)g;
}

/***********************************************************************************/

/*********************
** The SizeButtonClass
*********************/

#ifdef SKINS
#define SIZETYPE_RIGHTBOTTOM 1
#define SIZETYPE_RIGHT       2
#define SIZETYPE_BOTTOM      3
#define SIZETYPE_LEFTBOTTOM  4
#define SIZETYPE_LEFT        5
#define SIZETYPE_LEFTTOP     6
#define SIZETYPE_TOP         7
#define SIZETYPE_RIGHTTOP    8
#endif

/***********************************************************************************/

void smartresize(struct Window *w,struct sizebutton_data *data,Class *cl)
{
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    LockLayers(&w->WScreen->LayerInfo);
    
    if (BLAYER(w))
    {
        struct Hook *backfill;

        backfill = BLAYER(w)->BackFill;
        BLAYER(w)->BackFill = LAYERS_NOBACKFILL;

        /* move outer window first */
        MoveSizeLayer(BLAYER(w), data->left - w->LeftEdge, data->top - w->TopEdge, data->width - w->Width, data->height - w->Height);

        BLAYER(w)->BackFill = backfill;
    }

    {
        struct Hook *backfill;

        backfill = WLAYER(w)->BackFill;
        WLAYER(w)->BackFill = LAYERS_NOBACKFILL;
        MoveSizeLayer(WLAYER(w), data->left - w->LeftEdge, data->top - w->TopEdge, data->width - w->Width, data->height - w->Height);
        WLAYER(w)->BackFill = backfill;
    }

    w->TopEdge = data->top;
    w->LeftEdge = data->left;
    w->Width = data->width;
    w->Height = data->height;

    IW(w)->specialflags |= SPFLAG_LAYERRESIZED;

    if ((iihdata->ActiveGadget) && (w == iihdata->GadgetInfo.gi_Window))
    {
        GetGadgetDomain(iihdata->ActiveGadget,
                iihdata->GadgetInfo.gi_Screen,
                iihdata->GadgetInfo.gi_Window,
                NULL,
                &iihdata->GadgetInfo.gi_Domain);
    }

    /* Relayout GFLG_REL??? gadgets */
    DoGMLayout(w->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);

    ih_fire_intuimessage(w,
                     IDCMP_NEWSIZE,
                     0,
                     w,
                     IntuitionBase);

    CheckLayers(w->WScreen, IntuitionBase);

    UnlockLayers(&w->WScreen->LayerInfo);
}

/***********************************************************************************/

IPTR SizeButtonClass__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct InputEvent   *ie = msg->gpi_IEvent;
    IPTR            	 retval = GMR_NOREUSE;

    if (ie)
    {
        /* The gadget was activated via mouse input */
        struct sizebutton_data  *data;
        struct Window       	*w;

        /* There is no point in rerendering ourseleves her, as this
           is done by a call to RefreshWindowFrame() in the intuition inputhandler
        */

        w = msg->gpi_GInfo->gi_Window;

        data = INST_DATA(cl, g);

    #ifdef USEWINDOWLOCK
        /* do NOT ObtainSemaphore here since this would lead to deadlocks!!! */
        /* when the semaphore can not be obtained we simply ignore gadget activation */
        if (!(AttemptSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock)))
        {
            goto fail;
        }
        data->drag_windowlock = TRUE;
    #endif

        data->height = data->Height = w->Height;
        data->width  = data->Width = w->Width;
        data->left = data->LeftEdge = w->LeftEdge;
        data->top = data->TopEdge = w->TopEdge;

        data->mouseoffsetx = w->WScreen->MouseX;
        data->mouseoffsety = w->WScreen->MouseY;

        data->drag_refreshed = TRUE;
        data->drag_ticks = 2;

    #ifdef SKINS
        data->drag_type = 0;

        if (w->MouseX < IW(w)->sizeimage_width)
        {
            if (w->MouseY < IW(w)->sizeimage_height) data->drag_type = SIZETYPE_LEFTTOP;
            if (w->MouseY > w->Height - IW(w)->sizeimage_height - 1) data->drag_type = SIZETYPE_LEFTBOTTOM;
            if (!data->drag_type) data->drag_type = SIZETYPE_LEFT;
        }

        if ((!data->drag_type) && (w->MouseX >= IW(w)->sizeimage_width) && (w->MouseX <= w->Width - 1 - IW(w)->sizeimage_width))
        {
            if (w->MouseY < w->BorderTop)
            {
                data->drag_type = SIZETYPE_TOP;
            } else {
                data->drag_type = SIZETYPE_BOTTOM;
            }
        }

        if ((!data->drag_type) && (w->MouseX > IW(w)->sizeimage_width))
        {
            if (w->MouseY < IW(w)->sizeimage_height) data->drag_type = SIZETYPE_RIGHTTOP;
            if (w->MouseY > w->Height - IW(w)->sizeimage_height - 1) data->drag_type = SIZETYPE_RIGHTBOTTOM;
            if (!data->drag_type) data->drag_type = SIZETYPE_RIGHT;
        }

        if (!data->drag_type) goto fail;
    #endif

        data->rp = CloneRastPort(&w->WScreen->RastPort);
        if (data->rp)
        {
            /* Lock all layers while the window is resized.
             * Get the gadget lock first to avoid deadlocks
             * with ObtainGIRPort. */

            if (!OPAQUESIZE)
            {
    	    #ifdef USEGADGETLOCK
                if (AttemptSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock))
                {
                    data->drag_gadgetlock = TRUE;
                }
                else
                {
                    goto fail;
                }
    	    #endif
    	    #ifndef DELAYEDSIZE
                LockLayers(&w->WScreen->LayerInfo);
                data->drag_layerlock = TRUE;
    	    #endif
            }

            SetDrMd(data->rp, COMPLEMENT);

    	#ifndef DELAYEDSIZE
            if(!OPAQUESIZE)
                drawwindowframe(w->WScreen
                        	, data->rp
                        	, data->left
                        	, data->top
                        	, data->left + data->width  - 1
                        	, data->top  + data->height - 1
                        	, IntuitionBase
                                );

            data->isrendered = TRUE;
    	#endif

            {
                UQUAD currenttime;
		
                currenttime = ie->ie_TimeStamp.tv_secs;
                currenttime = currenttime * 50;
                currenttime += ie->ie_TimeStamp.tv_micro / 20000;
                data->lasteventtime = currenttime;
            }

            data->drag_canceled = FALSE;

            return GMR_MEACTIVE;
        }

fail:
        if (data->drag_layerlock)
        {
            UnlockLayers(&w->WScreen->LayerInfo);
            data->drag_layerlock = FALSE;
            ;
        }

#ifdef USEGADGETLOCK
        if (data->drag_gadgetlock)
        {
            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
            data->drag_gadgetlock = FALSE;
        }
#endif

#ifdef USEWINDOWLOCK
        if (data->drag_windowlock)
        {
            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
            data->drag_windowlock = FALSE;
        }
#endif
    }

    return retval;

}

/***********************************************************************************/

IPTR SizeButtonClass__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct GadgetInfo   *gi = msg->gpi_GInfo;
    IPTR            	 retval = GMR_MEACTIVE;

    if (gi)
    {
        struct InputEvent   	*ie = msg->gpi_IEvent;
        struct sizebutton_data  *data = INST_DATA(cl, g);
        struct Window       	*w = msg->gpi_GInfo->gi_Window;

        switch (ie->ie_Class)
        {
        case IECLASS_RAWMOUSE:
            switch (ie->ie_Code)
            {
            case SELECTUP:
                retval = GMR_NOREUSE;
                break;


            case IECODE_NOBUTTON:
                {
                    struct Screen   *scr = w->WScreen;
                    LONG             new_width = 0;
                    LONG             new_height = 0;

                    /* Can we move to the new position, or is window at edge of display ? */
    	    	#ifdef SKINS
                    switch(data->drag_type)
                    {
                        case SIZETYPE_BOTTOM:
                            new_height  = data->Height + scr->MouseY - data->mouseoffsety;
                            new_width   = data->Width;
                            break;

                        case SIZETYPE_TOP:
                            new_height  = data->Height + data->mouseoffsety - scr->MouseY;
                            new_width = data->Width;
                            break;

                        case SIZETYPE_RIGHTBOTTOM:
                            new_width   = data->Width + scr->MouseX - data->mouseoffsetx;
                            new_height  = data->Height + scr->MouseY - data->mouseoffsety;
                            break;

                        case SIZETYPE_LEFTTOP:
                            new_width   = data->Width + data->mouseoffsetx - scr->MouseX;
                            new_height  = data->Height + data->mouseoffsety - scr->MouseY;
                            break;

                        case SIZETYPE_RIGHTTOP:
                            new_width   = data->Width + scr->MouseX - data->mouseoffsetx;
                            new_height  = data->Height + data->mouseoffsety - scr->MouseY;
                            break;

                        case SIZETYPE_LEFTBOTTOM:
                            new_width   = data->Width + data->mouseoffsetx - scr->MouseX;
                            new_height  = data->Height + scr->MouseY - data->mouseoffsety;
                            break;

                        case SIZETYPE_LEFT:
                            new_width   = data->Width + data->mouseoffsetx - scr->MouseX;
                            new_height = data->Height;
                            break;

                        case SIZETYPE_RIGHT:
                            new_width   = data->Width + scr->MouseX - data->mouseoffsetx;
                            new_height = data->Height;
                            break;
                    }
    	    	#else
                    new_width   = data->Width + scr->MouseX - data->mouseoffsetx;
                    new_height  = data->Height + scr->MouseY - data->mouseoffsety;
    	    	#endif
                    if (new_width < 0)
                        new_width = 1;

                    if (w->MinWidth != 0 && new_width < (ULONG)w->MinWidth)
                        new_width = w->MinWidth;

                    if (w->MaxWidth != 0 && new_width > (ULONG)w->MaxWidth)
                        new_width = w->MaxWidth;

                    if (new_height < 0)
                        new_height = 1;

                    if (w->MinHeight != 0 && new_height < (ULONG)w->MinHeight)
                        new_height = w->MinHeight;

                    if (w->MaxHeight != 0 && new_height > (ULONG)w->MaxHeight)
                        new_height = w->MaxHeight;


                    #ifdef SKINS
                    if (!((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS) && (w->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS)))
                    {
                        /* limit dimensions so window fits on the screen */
                        switch (data->drag_type)
                        {
                            case SIZETYPE_RIGHT:
                            case SIZETYPE_RIGHTTOP:
                            case SIZETYPE_RIGHTBOTTOM:
                                if (data->left + new_width > scr->Width)
                                    new_width = scr->Width - data->left;
                                break;

                            case SIZETYPE_LEFT:
                            case SIZETYPE_LEFTBOTTOM:
                            case SIZETYPE_LEFTTOP:
                                if (data->LeftEdge + data->Width - new_width < 0)
                                    new_width += (data->LeftEdge + data->Width - new_width);
                                break;
                        }

                        switch (data->drag_type)
                        {
                            case SIZETYPE_LEFTBOTTOM:
                            case SIZETYPE_BOTTOM:
                            case SIZETYPE_RIGHTBOTTOM:
                                if (data->top + new_height > scr->Height)
                                    new_height = scr->Height - data->top;
                                break;

                            case SIZETYPE_TOP:
                            case SIZETYPE_LEFTTOP:
                            case SIZETYPE_RIGHTTOP:
                                if (data->TopEdge + data->Height - new_height < 0)
                                    new_height += (data->TopEdge + data->Height - new_height);
                                break;
                        }
                    }
                    #endif

                    if (data->height != new_height || data->width != new_width)
                    {
                        SetDrMd(data->rp, COMPLEMENT);

                        if (data->isrendered && !OPAQUESIZE)
                        {
                            /* Erase old frame */
                            drawwindowframe(w->WScreen
                                	    , data->rp
                                	    , data->left
                                	    , data->top
                                	    , data->left + data->width  - 1
                                	    , data->top  + data->height - 1
                                	    , IntuitionBase
                                	    );

                        }

                        #ifdef SKINS
                        switch(data->drag_type)
                        {
                            case SIZETYPE_LEFT:
                                data->left = data->LeftEdge + data->Width - new_width;
                                break;

                            case SIZETYPE_LEFTTOP:
                                data->left = data->LeftEdge + data->Width - new_width;
                                data->top = data->TopEdge + data->Height - new_height;
                                break;

                            case SIZETYPE_LEFTBOTTOM:
                                data->left = data->LeftEdge + data->Width - new_width;
                                break;

                            case SIZETYPE_RIGHTTOP:
                                data->top = data->TopEdge + data->Height - new_height;
                                break;

                            case SIZETYPE_TOP:
                                data->top = data->TopEdge + data->Height - new_height;
                                break;
                        }
                        #endif

                        data->width   = new_width;
                        data->height  = new_height;

                        /* Rerender the window frame */

                        #ifdef DELAYEDSIZE
                        if (!data->drag_layerlock && !OPAQUESIZE)
                        {
                            LockLayers(&w->WScreen->LayerInfo);
                            data->drag_layerlock = TRUE;
                        }
                        #endif

                        data->drag_refreshed = FALSE;
                        data->drag_ticks = 2;

                        if (!OPAQUESIZE)
                            drawwindowframe(w->WScreen
                                	    , data->rp
                                	    , data->left
                                	    , data->top
                                	    , data->left + data->width  - 1
                                	    , data->top  + data->height - 1
                                	    , IntuitionBase
                                	    );

                        data->isrendered = TRUE;

                    }

                    retval = GMR_MEACTIVE;

                    break;
                }

            default:
                retval = GMR_NOREUSE;
                data->drag_canceled = TRUE;
                break;



            } /* switch (ie->ie_Code) */
            break;

        case IECLASS_TIMER:
            #ifdef DELAYEDSIZE
            if (!data->drag_layerlock && !OPAQUESIZE)
            {
                UQUAD currenttime;
                currenttime = ie->ie_TimeStamp.tv_secs;
                currenttime = currenttime * 50;
                currenttime += ie->ie_TimeStamp.tv_micro / 20000;

                if (currenttime > data->lasteventtime + 10)
                {
                    LockLayers(&w->WScreen->LayerInfo);
                    data->drag_layerlock = TRUE;

                    drawwindowframe(w->WScreen
                        	    , data->rp
                        	    , data->left
                        	    , data->top
                        	    , data->left + data->width  - 1
                        	    , data->top  + data->height - 1
                        	    , IntuitionBase
                        	    );
                    data->lasteventtime = currenttime;
                    data->isrendered = TRUE;
                }
            }
            #endif

    	#if USE_OPAQUESIZE
            if (OPAQUESIZE)
            {
                data->drag_ticks --;
                if (!data->drag_refreshed && !data->drag_ticks && WindowsReplied(w->WScreen,IntuitionBase))
                {
                    smartresize(w,data,cl);
                    data->drag_refreshed = TRUE;
                    data->drag_ticks = 2;
                }
            }
    	#endif /* USE_OPAQUESIZE */

            break;

    #ifdef __MORPHOS__
        case IECLASS_NEWTIMER:
            if (OPAQUESIZE && !data->drag_refreshed && WindowsReplied(w->WScreen,IntuitionBase))
            {
                smartresize(w,data,cl);
                data->drag_refreshed = TRUE;
                data->drag_ticks = 2;
            }
            break;
    #endif /* __MORPHOS__ */
    
        } /* switch (ie->ie_Class) */

    } /* if (gi) */
    
    return retval;
}

/***********************************************************************************/

IPTR SizeButtonClass__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct sizebutton_data  *data;
    struct Window           *w;

    data = INST_DATA(cl, g);
    w = msg->gpgi_GInfo->gi_Window;

    /* Allways clear last drawn frame */

    if (data->isrendered && data->rp)
    {

        SetDrMd(data->rp, COMPLEMENT);

        /* Erase old frame */
        if (!OPAQUESIZE)
            drawwindowframe(w->WScreen
                	    , data->rp
                	    , data->left
                	    , data->top
                	    , data->left + data->width  - 1
                	    , data->top  + data->height - 1
                	    , IntuitionBase
                	    );

    }
    else
    {
        if (!OPAQUESIZE)
        {
            data->drag_canceled = TRUE;
        }
    }
    data->isrendered = FALSE;

    if (data->drag_layerlock)
    {
        UnlockLayers(&w->WScreen->LayerInfo);
        data->drag_layerlock = FALSE;
    }

#ifdef USEGADGETLOCK
    if (data->drag_gadgetlock)
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
        data->drag_gadgetlock = FALSE;
    }
#endif

#ifdef USEWINDOWLOCK
    if (data->drag_windowlock)
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
        data->drag_windowlock = FALSE;
    }
#endif

    //jDc: workarounds refresh pb on GZZ window resize
    ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->ActiveSysGadget->Flags &= ~GFLG_SELECTED;

    if (!data->drag_canceled || OPAQUESIZE)
    {
        if (OPAQUESIZE && data->drag_canceled)
        {
            DoMoveSizeWindow(w,data->LeftEdge,data->TopEdge,data->Width,data->Height,TRUE,IntuitionBase);
        }
	else
	{
            DoMoveSizeWindow(w,data->left,data->top,data->width,data->height,TRUE,IntuitionBase);
        }
        //ChangeWindowBox(w,data->left,data->top,data->width,data->height);
    }
    data->drag_canceled = TRUE;
    
    /* User throught with drag operation. Unlock layesr and free
    rastport clone
    */
    if (data->rp)
    {
        FreeRastPort(data->rp);
        data->rp = NULL;
    }

    return TRUE;
}

/***********************************************************************************/

IPTR SizeButtonClass__OM_NEW(Class *cl, Object *o, Msg msg)
{
    struct Gadget *g = DoSuperMethodA(cl, o, msg);
    if (g)
    {
	g->GadgetType |= GTYP_SYSGADGET | GTYP_SIZING;
    }
    
    return (IPTR)g;
}

/***********************************************************************************/

