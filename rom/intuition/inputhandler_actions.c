/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Responsible for executing deferred Intuition actions like
          MoveWindow, SizeWindow, ActivateWindow, etc.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <devices/inputevent.h>
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "maybe_boopsi.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define LOCK_REFRESH(x)		ObtainSemaphore(&GetPrivScreen(x)->RefreshLock)
#define UNLOCK_REFRESH(x)	ReleaseSemaphore(&GetPrivScreen(x)->RefreshLock)

#define LOCK_ACTIONS()      	ObtainSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);	
#define UNLOCK_ACTIONS()	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);

static void move_family(struct Window *, int , int);

/*******************************************************************************************************/

static void CheckLayerRefresh(struct Layer *lay, struct Screen *targetscreen,
			      struct IntuitionBase *IntuitionBase)
{   
    if (lay->Flags & LAYERREFRESH)
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
	    if (IS_GZZWINDOW(win) && (lay == win->BorderRPort->Layer))
	    {
	        /* simply refresh that window's frame */

		Gad_BeginUpdate(lay, IntuitionBase);
	        RefreshWindowFrame(win);
	        lay->Flags &= ~LAYERREFRESH;
		Gad_EndUpdate(lay, TRUE, IntuitionBase);
	    }
	    else
	    {
	        WindowNeedsRefresh(win, IntuitionBase);
	    }
	}
	
    } /* if (lay->Flags & LAYERREFRESH) */
}

/*******************************************************************************************************/

static void CheckLayerRefreshBehind(struct Layer *lay, struct Screen *screen,
	    			    struct IntuitionBase *IntuitionBase)
{
    for(; lay; lay = lay->back)
    {
        CheckLayerRefresh(lay, screen, IntuitionBase);
    }
}

/*******************************************************************************************************/

static void CheckLayerRefreshInFront(struct Layer *lay, struct Screen *screen,
				     struct IntuitionBase *IntuitionBase)
{
    for(; lay; lay = lay->front)
    {
        CheckLayerRefresh(lay, screen, IntuitionBase);
    }
}

/*******************************************************************************************************/

static void WindowSizeWillChange(struct Window *targetwindow, WORD dx, WORD dy, 
				 struct IntuitionBase *IntuitionBase)
{
    /* Erase the old frame on the right/lower side if
       new size is bigger than old size
    */

    D(bug("********* WindowSizeWillChange ******** dx = %d  dy = %d\n", dx, dy));

    if ( ((dx > 0) && (targetwindow->BorderRight  > 0)) ||
	 ((dy > 0) && (targetwindow->BorderBottom > 0)) )
    {
        struct RastPort 	*rp = targetwindow->BorderRPort;
        struct Layer 		*L = rp->Layer;
        struct Rectangle 	rect;
        struct Region 		*oldclipregion;
        WORD 			ScrollX;
        WORD 			ScrollY;

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

            EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

	    OrRectRegion(L->DamageList, &rect);

	    L->Flags |= LAYERREFRESH;
        }

        if ((dy > 0) && (targetwindow->BorderBottom > 0))

        {
	    rect.MinX = 0;
	    rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
	    rect.MaxX = targetwindow->Width - 1;
	    rect.MaxY = targetwindow->Height - 1;

            EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

	    OrRectRegion(L->DamageList, &rect);

	    L->Flags |= LAYERREFRESH;
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

    EraseRelGadgetArea(targetwindow, FALSE, IntuitionBase);
	
}

/*******************************************************************************************************/

static void WindowSizeHasChanged(struct Window *targetwindow, WORD dx, WORD dy,
				 BOOL is_sizewindow, struct IntuitionBase *IntuitionBase)
{
    struct Layer *lay;
    
    D(bug("********* WindowSizeHasChanged ********\n"));

    /* Relayout GFLG_REL??? gadgets */
    DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

    /* Add the new area of all GFLG_REL??? gadgets to the damagelist, but
       don't EraseRect() as the gadgets will be re-rendered at their new
       position anyway */
    EraseRelGadgetArea(targetwindow, TRUE, IntuitionBase);

    /* If new size is smaller than old size add right/bottom
       frame to damagelist */
    if ( ((dx < 0) && (targetwindow->BorderRight  > 0)) ||
	 ((dy < 0) && (targetwindow->BorderBottom > 0)) )
    {
	struct Rectangle rect;

	lay = targetwindow->BorderRPort->Layer;

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

    } else {
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
	    
    //if (is_sizewindow)
    {
	/* Send IDCMP_NEWSIZE to resized window */

	ih_fire_intuimessage(targetwindow,
			     IDCMP_NEWSIZE,
			     0,
			     targetwindow,
			     IntuitionBase);
    }
    
    /* Send IDCMP_CHANGEWINDOW to resized window */

    ih_fire_intuimessage(targetwindow,
		    	 IDCMP_CHANGEWINDOW,
			 CWCODE_MOVESIZE,
			 targetwindow,
			 IntuitionBase);

    lay = targetwindow->WLayer;
    
    if (lay->Flags & LAYERREFRESH)
    {
	ih_fire_intuimessage(targetwindow,
			     IDCMP_REFRESHWINDOW,
			     0,
			     targetwindow,
			     IntuitionBase);
    }  
}

/*******************************************************************************************************/

static void DoMoveSizeWindow(struct Window *targetwindow, WORD NewLeftEdge, WORD NewTopEdge,
			     WORD NewWidth, WORD NewHeight, struct IntuitionBase *IntuitionBase)
{
    struct IntWindow 	*w 	     = (struct IntWindow *)targetwindow;
    struct Layer	*targetlayer = targetwindow->WLayer, *L;
    WORD		OldLeftEdge  = targetwindow->LeftEdge;
    WORD		OldTopEdge   = targetwindow->TopEdge;
    WORD 		OldWidth     = targetwindow->Width;
    WORD 		OldHeight    = targetwindow->Height;
    WORD 		pos_dx, pos_dy, size_dx, size_dy;
    		
    /* correct new window coords if necessary */

    FixWindowCoords(targetwindow, &NewLeftEdge, &NewTopEdge, &NewWidth, &NewHeight);

    D(bug("DoMoveSizeWindow to %d,%d %d x %d\n", NewLeftEdge, NewTopEdge, NewWidth, NewHeight));
    
    pos_dx  = NewLeftEdge - OldLeftEdge;
    pos_dy  = NewTopEdge  - OldTopEdge;
    size_dx = NewWidth    - OldWidth;
    size_dy = NewHeight   - OldHeight;

    LOCK_REFRESH(targetwindow->WScreen);

    if (pos_dx || pos_dy || size_dx || size_dy)
    {
    
	if (size_dx || size_dy)
	{
	    WindowSizeWillChange(targetwindow, size_dx, size_dy, IntuitionBase);
	}

	targetwindow->LeftEdge    += pos_dx;
	targetwindow->TopEdge     += pos_dy;
	targetwindow->RelLeftEdge += pos_dx;
	targetwindow->RelTopEdge  += pos_dy;
	
	targetwindow->Width     = NewWidth;
	targetwindow->Height    = NewHeight; 
	targetwindow->GZZWidth  = targetwindow->Width  - targetwindow->BorderLeft - targetwindow->BorderRight;
	targetwindow->GZZHeight = targetwindow->Height - targetwindow->BorderTop  - targetwindow->BorderBottom;
 
	/* check for GZZ window */
	if (IS_GZZWINDOW(targetwindow))
	{
	    /* move outer window first */
	    MoveSizeLayer(targetwindow->BorderRPort->Layer, pos_dx, pos_dy, size_dx, size_dy);
	}

	MoveSizeLayer(targetlayer, pos_dx, pos_dy, size_dx, size_dy);

	if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = OldLeftEdge;
	if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = OldTopEdge;
	if (w->ZipWidth    != ~0) w->ZipWidth    = OldWidth;
	if (w->ZipHeight   != ~0) w->ZipHeight   = OldHeight;

	if (pos_dx || pos_dy) {
		UpdateMouseCoords(targetwindow);
		if (HAS_CHILDREN(targetwindow))
			move_family(targetwindow, pos_dx, pos_dy);
	}
    } /* if (pos_dx || pos_dy || size_dx || size_dy) */
    
    if (size_dx || size_dy)
    {
	/* This func also takes care of sending IDCMP_CHANGEWINDOW
	   and IDCMP_REFRESHWINDOW to targetwindow, therefore ... */

	WindowSizeHasChanged(targetwindow, size_dx, size_dy, FALSE, IntuitionBase);

	/* ... start checking refresh behind targetwindow */

	L = targetwindow->BorderRPort->Layer->back;
    } else {
	/* Send IDCMP_NEWSIZE and DCMP_CHANGEWINDOW to resized window, even
	   if there was no resizing/position change at all. BGUI for example
	   relies on this! */

	ih_fire_intuimessage(targetwindow,
			     IDCMP_NEWSIZE,
			     0,
			     targetwindow,
			     IntuitionBase);

	ih_fire_intuimessage(targetwindow,
		    	     IDCMP_CHANGEWINDOW,
			     CWCODE_MOVESIZE,
			     targetwindow,
			     IntuitionBase);

	/* Also check targetwindow for refresh */

	L = targetlayer;
    }

    if ((size_dx < 0) || (size_dy < 0) || pos_dx || pos_dy)
    {
	CheckLayerRefreshBehind(L, targetwindow->WScreen, IntuitionBase);
    }
    
    UNLOCK_REFRESH(targetwindow->WScreen);

}

/*******************************************************************************************************/

void HandleIntuiActions(struct IIHData *iihdata,
			  struct IntuitionBase *IntuitionBase)
{
    struct IntuiActionMessage *am, *next_am;
    
    D(bug("Handle Intuition action messages\n"));
    
    LOCK_ACTIONS();

    am = (struct IntuiActionMessage *)iihdata->IntuiActionQueue.mlh_Head;
    next_am = (struct IntuiActionMessage *)am->ExecMessage.mn_Node.ln_Succ;
    
    UNLOCK_ACTIONS();

    /* Handle Intuition action messages */

    while(next_am)
    {
        struct Window * targetwindow = am->Window;
	struct Screen * targetscreen = targetwindow ? targetwindow->WScreen : NULL;
        struct Layer  * L, *targetlayer = targetwindow ? targetwindow->WLayer : NULL;
        BOOL CheckLayersBehind = FALSE;
        BOOL CheckLayersInFront = FALSE;
	BOOL remove_am = TRUE;
	BOOL free_am = TRUE;
	
	if (MENUS_ACTIVE &&
	    ((am->Code != AMCODE_CLOSEWINDOW) || (targetwindow == iihdata->MenuWindow)))
	{
	    remove_am = FALSE;
	    free_am = FALSE;
	    
	    goto next_action;
	}
			
 	switch (am->Code)
	{
	    case AMCODE_CLOSEWINDOW: {
		if (!IS_GZZWINDOW(targetwindow))
		{
		    /* not a GGZ window */
		    L = targetlayer->back;
		}
		else
		{
		    /* a GZZ window */
		    L = targetlayer->back->back;
		}

		LOCK_ACTIONS();
		Remove(&am->ExecMessage.mn_Node);
		UNLOCK_ACTIONS();
		
		remove_am = FALSE; /* because int_closewindow frees the message!!! */
		free_am = FALSE;
		
		/* if there is an active gadget in the window being closed, then make
		   it inactive */
		   
		if (iihdata->ActiveGadget &&
		    !IS_SCREEN_GADGET(iihdata->ActiveGadget) &&
		    (iihdata->GadgetInfo.gi_Window == targetwindow))
		{
		    struct Gadget *gadget = iihdata->ActiveGadget;
		    
		    switch(gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		        case GTYP_CUSTOMGADGET:
			    {
		    		struct gpGoInactive gpgi;

				gpgi.MethodID = GM_GOINACTIVE;
				gpgi.gpgi_GInfo = &iihdata->GadgetInfo;
				gpgi.gpgi_Abort = 1; 

				Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);
			    }
			    break;
			
			case GTYP_STRGADGET:
			case GTYP_BOOLGADGET:
			    gadget->Flags &= ~GFLG_SELECTED;
			    break;
		    }
		    
		    gadget->Activation &= ~GACT_ACTIVEGADGET;
		    
		    iihdata->ActiveGadget = NULL;
		    
		} /* if there's an active gadget in window which is being closed */
		
		LOCK_REFRESH(targetscreen);
		
		int_closewindow(am, IntuitionBase);
		CheckLayerRefreshBehind(L, targetscreen, IntuitionBase);
		
		UNLOCK_REFRESH(targetscreen);
	    break; }

	    case AMCODE_WINDOWTOFRONT: {
		if (!(targetlayer->Flags & LAYERBACKDROP))
		{
		    LOCK_REFRESH(targetscreen);
		    
		    /* GZZ or regular window? */
		    if (IS_GZZWINDOW(targetwindow))
		    {
			/* bring outer window to front first!! */

			UpfrontLayer(NULL, targetwindow->BorderRPort->Layer);
			if (targetwindow->BorderRPort->Layer->Flags & LAYERREFRESH)
			{
			    Gad_BeginUpdate(targetwindow->BorderRPort->Layer, IntuitionBase);
		            RefreshWindowFrame(targetwindow);
			    targetwindow->BorderRPort->Layer->Flags &= ~LAYERREFRESH;
			    Gad_EndUpdate(targetwindow->BorderRPort->Layer, TRUE, IntuitionBase);
			}
		    }

		    UpfrontLayer(NULL, targetlayer);

		    /* only this layer (inner window) needs to be updated */
		    if (targetlayer->Flags & LAYERREFRESH)
		    {
			Gad_BeginUpdate(targetlayer, IntuitionBase);
			
			if (!IS_GZZWINDOW(targetwindow))
			{
			    RefreshWindowFrame(targetwindow);
			}
			
			RefreshGadgets(targetwindow->FirstGadget, targetwindow, NULL);
			
			if (IS_NOCAREREFRESH(targetwindow))
			{
			    targetlayer->Flags &= ~LAYERREFRESH;
			}
			
			Gad_EndUpdate(targetlayer, IS_NOCAREREFRESH(targetwindow), IntuitionBase);

			if (!IS_NOCAREREFRESH(targetwindow))
			{
			    ih_fire_intuimessage(targetwindow,
			                	 IDCMP_REFRESHWINDOW,
						 0,
						 targetwindow,
						 IntuitionBase);
			}
			
		    } /* if (targetlayer->Flags & LAYERREFRESH) */
		    
		    UNLOCK_REFRESH(targetscreen);
		    
		} /* if (!(targetlayer->Flags & LAYERBACKDROP)) */
		
		NotifyDepthArrangement(targetwindow, IntuitionBase);
	    break; }

	    case AMCODE_WINDOWTOBACK: {
		/* I don't move backdrop layers! */
		if (!(targetlayer->Flags & LAYERBACKDROP))
		{

		    LOCK_REFRESH(targetscreen);
		    
		    BehindLayer(0, targetlayer);

		    /* GZZ window or regular window? */
		    if (IS_GZZWINDOW(targetwindow))
		    {
			/* move outer window behind! */
			/* attention: targetlayer->back would not be valid as
		                      targetlayer already moved!! 
			*/
			BehindLayer(0, targetwindow->BorderRPort->Layer);

			/* 
                	 * stegerg: check all layers including inner gzz
			 *          layer because of this: inner layer
			 *          was moved back first, so it gets 
			 *          completely under the outer layer.
			 *          Maybe it would be better to move the
			 *          outer layer first and then move the
			 *          inner layer with MoveLayerinfrontof
			 *          (outerlayer) ????
			 * 
                	 */

			L = targetlayer; /* targetlayer->front */
		    } else {

                	/* 
                	 * Check all layers in front of the layer.
                	 */
			L = targetlayer->front;
		    }
		    
		    CheckLayerRefreshInFront(L, targetscreen, IntuitionBase);
		    
		    UNLOCK_REFRESH(targetscreen);
		}

		NotifyDepthArrangement(targetwindow, IntuitionBase);
	    break; }

	    case AMCODE_ACTIVATEWINDOW: {		
		/* On the Amiga ActivateWindow is delayed if there
		   is an active gadget (altough this does not seem
		   to be true for string gadgets). We just ignore
		   ActivateWindow in such a case. */
		   
		if (!iihdata->ActiveGadget)
		{
		    struct Window *w = IntuitionBase->ActiveWindow;
		    
	    	    if(w)
		    {
		    	ih_fire_intuimessage(w,
					     IDCMP_INACTIVEWINDOW,
					     0,
					     w,
					     IntuitionBase);
		    }

		    int_activatewindow(targetwindow, IntuitionBase);
		    
		    ih_fire_intuimessage(targetwindow,
					 IDCMP_ACTIVEWINDOW,
					 0,
					 targetwindow,
					 IntuitionBase);
		    
		} /* if (!iihdata->ActiveGadget) */
		
	    break; }


            case AMCODE_MOVEWINDOW: {
	    	WORD dx = am->iam_MoveWindow.dx;
		WORD dy = am->iam_MoveWindow.dy;
		
		if (!(targetscreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
		{
		    /* correct dx, dy if necessary */

		    if ((targetwindow->LeftEdge + targetwindow->Width + dx) > targetwindow->WScreen->Width)
		    {
			dx = targetwindow->WScreen->Width - (targetwindow->LeftEdge + targetwindow->Width);
		    } else if ((targetwindow->LeftEdge + dx) < 0)
		    {
			dx = -targetwindow->LeftEdge;
		    }

		    if ((targetwindow->TopEdge + targetwindow->Height + dy) > targetwindow->WScreen->Height)
		    {
			dy = targetwindow->WScreen->Height - (targetwindow->TopEdge + targetwindow->Height);
		    } else if ((targetwindow->TopEdge + dy) < 0)
		    {
			dy = -targetwindow->TopEdge;
		    }
    	    	}
				
		if (dx || dy)
		{   
		    LOCK_REFRESH(targetscreen);
		    
                    MoveLayer(0, targetlayer, dx, dy);

                    /* in case of GZZ windows also move outer window */
                    if (IS_GZZWINDOW(targetwindow))
                    {
                	MoveLayer(NULL, targetwindow->BorderRPort->Layer, dx, dy);
                    }

                    targetwindow->LeftEdge += dx;
                    targetwindow->TopEdge  += dy;
                    targetwindow->RelLeftEdge += dx;
                    targetwindow->RelTopEdge  += dy;

                    if (HAS_CHILDREN(targetwindow))
                      move_family(targetwindow, dx, dy);

		    CheckLayerRefreshBehind(targetlayer, targetscreen, IntuitionBase);
		    
                    UNLOCK_REFRESH(targetscreen);

		    UpdateMouseCoords(targetwindow);
		    
		    /* Send IDCMP_CHANGEWINDOW to moved window */

		    ih_fire_intuimessage(targetwindow,
		    			 IDCMP_CHANGEWINDOW,
					 CWCODE_MOVESIZE,
					 targetwindow,
					 IntuitionBase);
					 
		} /* if (am->dx || am->dy) */
		
            break; }

            case AMCODE_MOVEWINDOWINFRONTOF: {
	        struct Window *BehindWindow = am->iam_MoveWindowInFrontOf.BehindWindow;
	        struct Layer  *lay;
	        BOOL 	      movetoback = TRUE;
		
		for(lay = BehindWindow->WLayer; lay; lay = lay->back)
		{
		    if (lay == targetwindow->WLayer)
		    {
		        movetoback = FALSE;
			break;
		    }
		}
		
		/* FIXXXXXXXXXXXXXXXXXXXXXXXXXXXX FIXME FIXXXXXXXXXXXXXXXXXX */
		
                /* If GZZ window then also move outer window */
		
		LOCK_REFRESH(targetscreen);
		
                if (IS_GZZWINDOW(targetwindow))
		{
                    MoveLayerInFrontOf(targetwindow->BorderRPort->Layer, BehindWindow->WLayer);
                }
                MoveLayerInFrontOf(targetwindow->WLayer, BehindWindow->WLayer);

                CheckLayersBehind = TRUE;
                L = targetlayer;

		UNLOCK_REFRESH(targetscreen);
		
		NotifyDepthArrangement(targetwindow, IntuitionBase);
            break; }

            case AMCODE_SIZEWINDOW: {
                WORD OldLeftEdge  = targetwindow->RelLeftEdge;
                WORD OldTopEdge   = targetwindow->RelTopEdge;
                WORD OldWidth     = targetwindow->Width;
                WORD OldHeight    = targetwindow->Height;
                WORD NewLeftEdge  = OldLeftEdge;
		WORD NewTopEdge   = OldTopEdge;
		WORD NewWidth	  = OldWidth  + am->iam_SizeWindow.dx;
		WORD NewHeight	  = OldHeight + am->iam_SizeWindow.dy;
		WORD size_dx, size_dy;

                /* correct new window coords if necessary */

		FixWindowCoords(targetwindow, &NewLeftEdge, &NewTopEdge, &NewWidth, &NewHeight);
		
		if (NewLeftEdge != OldLeftEdge)
		{
		    /* am->dx was too big */
		    NewLeftEdge = OldLeftEdge;
		    NewWidth    = targetwindow->WScreen->Width - NewLeftEdge;
		}
		if (NewTopEdge != OldTopEdge)
		{
		    /* am->dy was too big */
		    NewTopEdge = OldTopEdge;
		    NewHeight  = targetwindow->WScreen->Height - NewTopEdge;		    
		}

		size_dx = NewWidth - OldWidth;
		size_dy = NewHeight - OldHeight;

		if (!size_dx && !size_dy) break;
		
		LOCK_REFRESH(targetscreen);
		
		WindowSizeWillChange(targetwindow, size_dx, size_dy, IntuitionBase);

                targetwindow->Width  += size_dx;
                targetwindow->Height += size_dy;
		targetwindow->GZZWidth  = targetwindow->Width  - targetwindow->BorderLeft - targetwindow->BorderRight;
		targetwindow->GZZHeight = targetwindow->Height - targetwindow->BorderTop  - targetwindow->BorderBottom;

                /* I first resize the outer window if a GZZ window */
                if (IS_GZZWINDOW(targetwindow))
                {
                    SizeLayer(NULL, targetwindow->BorderRPort->Layer, size_dx, size_dy);
                }
                SizeLayer(NULL, targetlayer, size_dx, size_dy);

		WindowSizeHasChanged(targetwindow, size_dx, size_dy, TRUE, IntuitionBase);				

                /* 
                   Only if the window is smaller now there can be damage
                   to report to layers further behind.
                */
                if ((size_dx < 0) || (size_dy < 0))
                {
		    CheckLayerRefreshBehind(targetwindow->BorderRPort->Layer->back, targetscreen, IntuitionBase);
                }

		UNLOCK_REFRESH(targetscreen);
		
            break; }

            case AMCODE_ZIPWINDOW: {
                struct IntWindow * w = (struct IntWindow *)targetwindow;
                WORD NewLeftEdge, NewTopEdge, NewWidth, NewHeight;
		
		NewLeftEdge = targetwindow->LeftEdge;
		if (w->ZipLeftEdge != ~0)
		{
		    NewLeftEdge    = w->ZipLeftEdge;
		    w->ZipLeftEdge = w->window.RelLeftEdge;
		}

		NewTopEdge = targetwindow->TopEdge;
		if (w->ZipTopEdge != ~0)
		{
		    NewTopEdge    = w->ZipTopEdge;
		    w->ZipTopEdge = w->window.RelTopEdge;
		}
		
		NewWidth = targetwindow->Width;
		if (w->ZipWidth != ~0)
		{
		    NewWidth    = w->ZipWidth;
		    w->ZipWidth = w->window.Width;
		}
		
		NewHeight = targetwindow->Height;
		if (w->ZipHeight != ~0)
		{
		    NewHeight    = w->ZipHeight;
		    w->ZipHeight = w->window.Height;
		}

		DoMoveSizeWindow(targetwindow, NewLeftEdge, NewTopEdge, NewWidth, NewHeight, IntuitionBase);
		
            break; }

	    case AMCODE_CHANGEWINDOWBOX: {

		DoMoveSizeWindow(targetwindow,
				 am->iam_ChangeWindowBox.Left,
				 am->iam_ChangeWindowBox.Top,
				 am->iam_ChangeWindowBox.Width,
				 am->iam_ChangeWindowBox.Height,
				 IntuitionBase);
		
	    break; }
	
	    case AMCODE_ACTIVATEGADGET:
	    	/* Note: This message must not be freed, because
		   it must be replied back to the app by sending
		   SIGF_INTUITION to the app task and placing
		   a result code in am->Code!!! */

		LOCK_ACTIONS();
		Remove(&am->ExecMessage.mn_Node);
		UNLOCK_ACTIONS();
		
		remove_am = FALSE;
		free_am = FALSE;

		am->Code = FALSE;
		
		/* Activate gadget only if no other gadget is
		   actually active and the gadget to be
		   activated is in the actual active window
		   and the gadget is not disabled */
		   
		if ((iihdata->ActiveGadget == NULL) &&
		    (IntuitionBase->ActiveWindow == targetwindow) &&
		    ((am->iam_ActivateGadget.Gadget->Flags & GFLG_DISABLED) == 0))
		{

		    if (DoActivateGadget(targetwindow, am->iam_ActivateGadget.Gadget, IntuitionBase))
		    {
		    	am->Code = TRUE;
		    }		
		} /* if gadget activation ok (no other gadget active, window active, ...) */
		
		Signal(am->Task, SIGF_INTUITION);
		break;
	    
	    case AMCODE_NEWPREFS:
		/*
		** The preferences were changed and now I need to inform
		** all interested windows about this.
		*/
		notify_newprefs(IntuitionBase);
	        break;
	
	    case AMCODE_SCREENSHOWTITLE:
	    	targetscreen = am->iam_ShowTitle.Screen;
		if ((targetscreen->Flags & SHOWTITLE) && (am->iam_ShowTitle.ShowIt == FALSE))
		{
		    LOCK_REFRESH(targetscreen);
		    
		    BehindLayer(0, targetscreen->BarLayer);
		    
		    Forbid();
		    targetscreen->Flags &= ~SHOWTITLE;
		    Permit();
		    
		    L = targetscreen->BarLayer->front;
		    
		    CheckLayerRefreshInFront(targetscreen->BarLayer->front, targetscreen, IntuitionBase);
		    
		    UNLOCK_REFRESH(targetscreen);
		    
		} else if (!(targetscreen->Flags & SHOWTITLE) && (am->iam_ShowTitle.ShowIt == TRUE))
		{
		    UpfrontLayer(0, targetscreen->BarLayer);
		    
		    if (targetscreen->BarLayer->Flags & LAYERREFRESH)
		    {
		        RenderScreenBar(targetscreen, TRUE, IntuitionBase);
		    }
		    
		    Forbid();
		    targetscreen->Flags |= SHOWTITLE;
		    Permit();
		}
		break;
		
	    case AMCODE_SCREENDEPTH:
	        int_screendepth(am->iam_ScreenDepth.Screen, am->iam_ScreenDepth.Flags, IntuitionBase);
		break;
		

#ifdef ChangeLayerVisibility
	    case AMCODE_SHOWWINDOW:
	    	LOCK_REFRESH(targetscreen);
		
                if (IS_GZZWINDOW(targetwindow))
		{
                    ChangeLayerVisibility(targetwindow->BorderRPort->Layer, am->iam_ShowWindow.yesno);
                }
                ChangeLayerVisibility(targetwindow->WLayer, am->iam_ShowWindow.yesno);
		
		if (am->iam_ShowWindow.yesno)
		{
		    /* There can only be damage in the window which has been made visible */
		    
		    /* TODO: also check for refresh in child layers */
		    
		    if (IS_GZZWINDOW(targetwindow)) CheckLayerRefresh(targetwindow->BorderRPort->Layer, targetscreen, IntuitionBase);		    
	            CheckLayerRefresh(targetwindow->WLayer, targetscreen, IntuitionBase);
		}
		else
		{
		    /* Since the window has been made invisible, all the layers behind might have been damaged */
		    CheckLayerRefreshBehind(targetlayer->back, targetscreen, IntuitionBase);
		}
		
		UNLOCK_REFRESH(targetscreen);
	        break;
#endif

#ifdef ChangeLayerShape
    	    case AMCODE_CHANGEWINDOWSHAPE:
	    	/* Note: for now GZZ windows are not supported. See ChangeWindowShape */

		LOCK_ACTIONS();
		Remove(&am->ExecMessage.mn_Node);
		UNLOCK_ACTIONS();
		
		remove_am = FALSE;
		free_am = FALSE;

	    	LOCK_REFRESH(targetscreen);
		
		am->iam_ChangeWindowShape.shape = ChangeLayerShape(targetlayer,
		    	    	    	    	    	    	   am->iam_ChangeWindowShape.shape,
								   am->iam_ChangeWindowShape.callback);
		
		CheckLayerRefreshBehind(targetlayer, targetscreen, IntuitionBase);
		
		UNLOCK_REFRESH(targetscreen);
		
		Signal(am->Task, SIGF_INTUITION);
		break;
#endif
		
	}

 	/* targetwindow might be invalid here (AM_CLOSEWINDOW) !!!!!!!!! */
	
	if (TRUE == CheckLayersBehind)
	{
	    /* Walk through all layers behind including the layer L
	       and check whether a layer needs a refresh 
	    */ 
	    struct Layer * _L = L;

	    while (NULL != _L)
	    {
		CheckLayerRefresh(_L, targetscreen, IntuitionBase);
	        _L = _L->back;
 
	    } /* while (NULL != _L) */

	} /* if (TRUE == CheckLayersBehind) */

	if (TRUE == CheckLayersInFront)
	{
	    /* Walk through all layers in front of including the layer L
	       and check whether a layer needs a refresh 
	    */

	    if (TRUE == CheckLayersBehind)
		L = L->front; /* the layer L has already been checked */

	    while (NULL != L)
	    {  
                CheckLayerRefresh(L, targetscreen, IntuitionBase);
		L = L->front;

	    } /* while (NULL != L) */

	} /* if (TRUE == CheckLayersInFront) */

next_action:
    	LOCK_ACTIONS();	

	if (remove_am) Remove(&am->ExecMessage.mn_Node);
	if (free_am)   FreeIntuiActionMsg(am, IntuitionBase);

	am = next_am;
	next_am = (struct IntuiActionMessage *)am->ExecMessage.mn_Node.ln_Succ;

    	UNLOCK_ACTIONS();
	
    } /* while (next_am) */

}
			  

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
