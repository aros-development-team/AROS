#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */
#include <proto/exec.h>
#include <proto/boopsi.h>
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
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

void HandleDeferedActions(struct IIHData *iihdata,
			  struct IntuitionBase *IntuitionBase)
{
    struct DeferedActionMessage *am, *next_am;
    
    D(bug("Handle defered action messages\n"));
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);

    am = (struct DeferedActionMessage *)iihdata->IntuiDeferedActionQueue.mlh_Head;
    next_am = (struct DeferedActionMessage *)am->ExecMessage.mn_Node.ln_Succ;
    
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);

    /* Handle defered action messages */

    while(next_am)
    {
        struct Window * targetwindow = am->Window;
        struct Layer  * targetlayer = targetwindow->WLayer, *L;
        BOOL CheckLayersBehind = FALSE;
        BOOL CheckLayersInFront = FALSE;
	BOOL remove_am = TRUE;
	BOOL free_am = TRUE;
	
	if (MENUS_ACTIVE && (am->Code != AMCODE_CLOSEWINDOW))
	{
	    remove_am = FALSE;
	    free_am = FALSE;
	    
	    goto next_action;
	}
			
 	switch (am->Code)
	{
	    case AMCODE_CLOSEWINDOW: {
		if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		{
		  /* not a GGZ window */
		  L = targetlayer->back;
		}
		else
		{
		  /* a GZZ window */
		  L = targetlayer->back->back;
		}

		if (NULL != L)
		  CheckLayersBehind = TRUE;
		
		ObtainSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);
		Remove(&am->ExecMessage.mn_Node);
		ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);
		
		remove_am = FALSE; /* because int_closewindow frees the message!!! */
		free_am = FALSE;
		
		int_closewindow(am, IntuitionBase);
	    break; }

	    case AMCODE_WINDOWTOFRONT: {
		if (0 == (targetlayer->Flags & LAYERBACKDROP))
		{
		  /* GZZ or regular window? */
		  if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		  {
		    /* bring outer window to front first!! */

		    UpfrontLayer(NULL, targetwindow->BorderRPort->Layer);
		    if (targetwindow->BorderRPort->Layer->Flags & LAYERREFRESH)
		    {
			BeginUpdate(targetwindow->BorderRPort->Layer);
		        RefreshWindowFrame(targetwindow);
			EndUpdate(targetwindow->BorderRPort->Layer, TRUE);

			targetwindow->BorderRPort->Layer->Flags &= ~LAYERREFRESH;
		    }
		  }

		  UpfrontLayer(NULL, targetlayer);

		  /* only this layer (inner window) needs to be updated */
		  if (0 != (targetlayer->Flags & LAYERREFRESH))
		  {
		    /* stegerg */

		    BeginUpdate(targetlayer);
		    if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		    {
			RefreshWindowFrame(targetwindow);
		    }
		    RefreshGadgets(targetwindow->FirstGadget, targetwindow, NULL);
		    EndUpdate(targetlayer, FALSE);

		    targetlayer->Flags &= ~LAYERREFRESH;

		    ih_fire_intuimessage(targetwindow,
			                 IDCMP_REFRESHWINDOW,
					 0,
					 targetwindow,
					 IntuitionBase);

		  }
		} 
		
		NotifyDepthArrangement(targetwindow, IntuitionBase);
	    break; }

	    case AMCODE_WINDOWTOBACK: {
		/* I don't move backdrop layers! */
		if (0 == (targetlayer->Flags & LAYERBACKDROP))
		{

		  BehindLayer(0, targetlayer);


		  /* GZZ window or regular window? */
		  if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
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
		      CheckLayersInFront = TRUE;

		  } else {

                      /* 
                       * Check all layers in front of the layer.
                       */
		      L = targetlayer->front;
		      CheckLayersInFront = TRUE;
		  }
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
		struct IntWindow *w = (struct IntWindow *)targetwindow;
		
		/* correct dx, dy if necessary */
		
		if ((targetwindow->LeftEdge + targetwindow->Width + am->dx) > targetwindow->WScreen->Width)
		{
		    am->dx = targetwindow->WScreen->Width - (targetwindow->LeftEdge + targetwindow->Width);
		} else if ((targetwindow->LeftEdge + am->dx) < 0)
		{
		    am->dx = -targetwindow->LeftEdge;
		}

		if ((targetwindow->TopEdge + targetwindow->Height + am->dy) > targetwindow->WScreen->Height)
		{
		    am->dy = targetwindow->WScreen->Height - (targetwindow->TopEdge + targetwindow->Height);
		} else if ((targetwindow->TopEdge + am->dy) < 0)
		{
		    am->dy = -targetwindow->TopEdge;
		}
		
		if (am->dx || am->dy)
		{   
                    MoveLayer(0,
                              targetlayer,
                              am->dx,
                              am->dy);

                    /* in case of GZZ windows also move outer window */
                    if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                    {
                      MoveLayer(NULL,
                        	targetwindow->BorderRPort->Layer,
                        	am->dx,
                        	am->dy);
                      RefreshWindowFrame(targetwindow);
                    }


                    if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = targetwindow->LeftEdge;
                    if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = targetwindow->TopEdge;

                    targetwindow->LeftEdge += am->dx;
                    targetwindow->TopEdge  += am->dy;

                    CheckLayersBehind = TRUE;
                    L = targetlayer;

		    /* Send IDCMP_CHANGEWINDOW to moved window */

		    ih_fire_intuimessage(targetwindow,
		    			 IDCMP_CHANGEWINDOW,
					 CWCODE_MOVESIZE,
					 targetwindow,
					 IntuitionBase);

		} /* if (am->dx || am->dy) */
		
            break; }

            case AMCODE_MOVEWINDOWINFRONTOF: { 
                /* If GZZ window then also move outer window */
                if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                {
                  MoveLayerInFrontOf(targetwindow->BorderRPort->Layer,
                                     am->BehindWindow->WLayer);
                  RefreshWindowFrame(targetwindow);
                }
                MoveLayerInFrontOf(     targetwindow->WLayer,
                                   am->BehindWindow->WLayer);

                CheckLayersBehind = TRUE;
                //CheckLayersInFront = TRUE;
                L = targetlayer;

		NotifyDepthArrangement(targetwindow, IntuitionBase);
            break; }

            case AMCODE_SIZEWINDOW: {
		/* correct dx, dy if necessary */

		if ((targetwindow->LeftEdge + targetwindow->Width + am->dx) > targetwindow->WScreen->Width)
		{
		   am->dx = targetwindow->WScreen->Width -
			    targetwindow->Width -
			    targetwindow->LeftEdge;
		}
		if ((targetwindow->TopEdge + targetwindow->Height + am->dy) > targetwindow->WScreen->Height)
		{
		   am->dy = targetwindow->WScreen->Height -
			    targetwindow->Height -
			    targetwindow->TopEdge;
		}

                /* First erase the old frame on the right side and 
                   on the lower side if necessary, but only do this
                   for non-GZZ windows 
                */

                if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                {
                  struct RastPort * rp = targetwindow->BorderRPort;
                  struct Layer * L = rp->Layer;
                  struct Rectangle rect;
                  struct Region * oldclipregion;
                  WORD ScrollX;
                  WORD ScrollY;
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

                  if ((am->dy > 0) && (targetwindow->BorderBottom > 0))

                  {
		    rect.MinX = targetwindow->BorderLeft;
		    rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - 1;

                    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
		    }
                  }

                  if ((am->dx > 0) && (targetwindow->BorderRight > 0))
                  {
		    rect.MinX = targetwindow->Width - targetwindow->BorderRight;
		    rect.MinY = targetwindow->BorderTop;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - targetwindow->BorderBottom;

                    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
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
                }

		/* Before resizing the layers eraserect the area of all
		   GFLG_REL*** gadgets (except those in the window border */
	        EraseRelGadgetArea(targetwindow, IntuitionBase);

                ((struct IntWindow *)targetwindow)->ZipWidth  = targetwindow->Width;
                ((struct IntWindow *)targetwindow)->ZipHeight = targetwindow->Height;

                targetwindow->Width += am->dx;
                targetwindow->Height+= am->dy;

                /* I first resize the outer window if a GZZ window */
                if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                {
                  SizeLayer(NULL,
                            targetwindow->BorderRPort->Layer,
                            am->dx,
                            am->dy);
                }

                SizeLayer(NULL, 
                          targetlayer,
                          am->dx,
                          am->dy);

                /* 
                   Only if the window is smaller now there can be damage
                   to report to layers further behind.
                */
                if (am->dx < 0 || am->dy < 0)
                {
                  CheckLayersBehind = TRUE;
                  if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                  {
                    L = targetlayer;
                  }
                  else
                  {
                    L = targetlayer->back;
                  }
                }
		/* Send GM_LAYOUT to all GA_RelS???? BOOPSI gadgets */

		DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

                /* and redraw the window frame */
		RefreshWindowFrame(targetwindow);

		/* and refresh all gadgets except border gadgets */
		int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
		
		/* Send IDCMP_NEWSIZE to resized window */

		ih_fire_intuimessage(targetwindow,
			             IDCMP_NEWSIZE,
				     0,
				     targetwindow,
				     IntuitionBase);

		/* Send IDCMP_CHANGEWINDOW to resized window */
		    
		ih_fire_intuimessage(targetwindow,
		    		     IDCMP_CHANGEWINDOW,
				     CWCODE_MOVESIZE,
				     targetwindow,
				     IntuitionBase);

            break; }

            case AMCODE_ZIPWINDOW: {
                struct IntWindow * w = (struct IntWindow *)targetwindow;
                WORD OldLeftEdge  = targetwindow->LeftEdge;
                WORD OldTopEdge   = targetwindow->TopEdge;
                WORD OldWidth     = targetwindow->Width;
                WORD OldHeight    = targetwindow->Height;
                WORD NewLeftEdge, NewTopEdge, NewWidth, NewHeight;
		WORD size_dx, size_dy;
		
		NewLeftEdge = OldLeftEdge;
		if (w->ZipLeftEdge != ~0) NewLeftEdge = w->ZipLeftEdge;

		NewTopEdge = OldTopEdge;
		if (w->ZipTopEdge != ~0) NewTopEdge = w->ZipTopEdge;

		NewWidth = OldWidth;
		if (w->ZipWidth != ~0) NewWidth = w->ZipWidth;

		NewHeight = OldHeight;
		if (w->ZipHeight != ~0) NewHeight = w->ZipHeight;

                /* correct new window coords if necessary */

		FixWindowCoords(targetwindow, &NewLeftEdge, &NewTopEdge, &NewWidth, &NewHeight);
		
		size_dx = NewWidth - OldWidth;
		size_dy = NewHeight - OldHeight;
		
		/* First erase the old frame on the right side and 
        	   on the lower side if necessary, but only do this
        	   for non-GZZ windows 
		*/

		if ((size_dx || size_dy) && (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO)))
		{
		  struct RastPort * rp = targetwindow->BorderRPort;
		  struct Layer * L = rp->Layer;
		  struct Rectangle rect;
		  struct Region * oldclipregion;
		  WORD ScrollX;
		  WORD ScrollY;

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

		  if ((size_dy > 0) && (targetwindow->BorderBottom > 0))

		  {
		    rect.MinX = targetwindow->BorderLeft;
		    rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - 1;

        	    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
		    }
		  }

		  if ((size_dx > 0) && (targetwindow->BorderRight > 0))
		  {
		    rect.MinX = targetwindow->Width - targetwindow->BorderRight;
		    rect.MinY = targetwindow->BorderTop;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - targetwindow->BorderBottom;

        	    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
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
		}

		if (size_dx || size_dy)
		{
		    /* Before resizing the layers eraserect the area of all
		       GFLG_REL*** gadgets (except those in the window border */
		    EraseRelGadgetArea(targetwindow, IntuitionBase);
		}

                targetwindow->LeftEdge = NewLeftEdge;
                targetwindow->TopEdge  = NewTopEdge;
                targetwindow->Width    = NewWidth;
                targetwindow->Height   = NewHeight; 

                /* check for GZZ window */
                if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                {
                  /* move outer window first */
                  MoveSizeLayer(targetwindow->BorderRPort->Layer,
                                NewLeftEdge - OldLeftEdge,
                                NewTopEdge  - OldTopEdge,
                                size_dx,
                                size_dy);
                }

                L = targetlayer;
                CheckLayersBehind = TRUE;

                MoveSizeLayer(targetlayer,
                              NewLeftEdge - OldLeftEdge,
                              NewTopEdge  - OldTopEdge,
                              NewWidth    - OldWidth,
                              NewHeight   - OldHeight);

                if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = OldLeftEdge;
                if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = OldTopEdge;
                if (w->ZipWidth  != ~0) w->ZipWidth    = OldWidth;
                if (w->ZipHeight != ~0) w->ZipHeight   = OldHeight;

		if (size_dx || size_dy)
		{
		    /* Send GM_LAYOUT to all GA_Rel??? BOOPSI gadgets */
		    DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

		    /* and redraw the window frame */
		    RefreshWindowFrame(targetwindow);

		    /* and refresh all gadgets except border gadgets */
		    int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
		}
		
		/* Send IDCMP_CHANGEWINDOW to resized window */

		ih_fire_intuimessage(targetwindow,
			             IDCMP_CHANGEWINDOW,
				     CWCODE_MOVESIZE,
				     targetwindow,
				     IntuitionBase);

            break; }

	    case AMCODE_CHANGEWINDOWBOX: {
                struct IntWindow * w = (struct IntWindow *)targetwindow;
                WORD OldLeftEdge  = targetwindow->LeftEdge;
                WORD OldTopEdge   = targetwindow->TopEdge;
                WORD OldWidth     = targetwindow->Width;
                WORD OldHeight    = targetwindow->Height;
                WORD NewLeftEdge  = am->left;
		WORD NewTopEdge   = am->top;
		WORD NewWidth	  = am->width;
		WORD NewHeight	  = am->height;
		WORD size_dx, size_dy;
		
                /* correct new window coords if necessary */

		FixWindowCoords(targetwindow, &NewLeftEdge, &NewTopEdge, &NewWidth, &NewHeight);
		
		size_dx = NewWidth - OldWidth;
		size_dy = NewHeight - OldHeight;
		
		/* First erase the old frame on the right side and 
        	   on the lower side if necessary, but only do this
        	   for non-GZZ windows 
		*/

		if ((size_dx || size_dy) && (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO)))
		{
		  struct RastPort * rp = targetwindow->BorderRPort;
		  struct Layer * L = rp->Layer;
		  struct Rectangle rect;
		  struct Region * oldclipregion;
		  WORD ScrollX;
		  WORD ScrollY;

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

		  if ((size_dy > 0) && (targetwindow->BorderBottom > 0))

		  {
		    rect.MinX = targetwindow->BorderLeft;
		    rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - 1;

        	    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
		    }
		  }

		  if ((size_dx > 0) && (targetwindow->BorderRight > 0))
		  {
		    rect.MinX = targetwindow->Width - targetwindow->BorderRight;
		    rect.MinY = targetwindow->BorderTop;
		    rect.MaxX = targetwindow->Width - 1;
		    rect.MaxY = targetwindow->Height - targetwindow->BorderBottom;

        	    EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		    if (0 != (L->Flags & LAYERSIMPLE))
		    {
			OrRectRegion(L->DamageList, &rect);
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
		}

		if (size_dx || size_dy)
		{
		    /* Before resizing the layers eraserect the area of all
		       GFLG_REL*** gadgets (except those in the window border */
		    EraseRelGadgetArea(targetwindow, IntuitionBase);
		}

                targetwindow->LeftEdge = NewLeftEdge;
                targetwindow->TopEdge  = NewTopEdge;
                targetwindow->Width    = NewWidth;
                targetwindow->Height   = NewHeight; 

                /* check for GZZ window */
                if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                {
                  /* move outer window first */
                  MoveSizeLayer(targetwindow->BorderRPort->Layer,
                                NewLeftEdge - OldLeftEdge,
                                NewTopEdge  - OldTopEdge,
                                size_dx,
                                size_dy);
             	}

                L = targetlayer;
                CheckLayersBehind = TRUE;

                MoveSizeLayer(targetlayer,
                              NewLeftEdge - OldLeftEdge,
                              NewTopEdge  - OldTopEdge,
                              NewWidth    - OldWidth,
                              NewHeight   - OldHeight);

                if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = OldLeftEdge;
                if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = OldTopEdge;
                if (w->ZipWidth  != ~0) w->ZipWidth    = OldWidth;
                if (w->ZipHeight != ~0) w->ZipHeight   = OldHeight;

		if (size_dx || size_dy)
		{
		    /* Send GM_LAYOUT to all GA_Rel??? BOOPSI gadgets */
		    DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

		    /* and redraw the window frame */
		    RefreshWindowFrame(targetwindow);

		    /* and refresh all gadgets except border gadgets */
		    int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
		}
		
		/* Send IDCMP_CHANGEWINDOW to resized window */

		ih_fire_intuimessage(targetwindow,
			             IDCMP_CHANGEWINDOW,
				     CWCODE_MOVESIZE,
				     targetwindow,
				     IntuitionBase);

		break; }
	
	    case AMCODE_ACTIVATEGADGET:
	    	/* Note: This message must not be freed, because
		   it must be replied back to the app by sending
		   SIGF_INTUITION to the app task and placing
		   a result code in am->Code!!! */

		am->Code = FALSE;
		
		/* Activate gadget only if no other gadget is
		   actually active and the gadget to be
		   activated is in the actual active window
		   and the gadget is not disabled */
		   
		if ((iihdata->ActiveGadget == NULL) &&
		    (IntuitionBase->ActiveWindow == targetwindow) &&
		    ((am->Gadget->Flags & GFLG_DISABLED) == 0))
		{

		    if (DoActivateGadget(targetwindow, am->Gadget, IntuitionBase))
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
	}

	if (TRUE == CheckLayersBehind)
	{
	  /* Walk through all layers behind including the layer L
	     and check whether a layer needs a refresh 
	  */ 
	  struct Layer * _L = L;
	  
	  while (NULL != _L)
	  {
	    /* Does this Layer need a refresh and does it belong
	       to a Window ?? */
	    if (0 != (_L->Flags & LAYERREFRESH))
	    {
	      if (_L == targetwindow->WScreen->BarLayer)
	      {
	        RenderScreenBar(targetwindow->WScreen, TRUE, IntuitionBase);
	      } else if (_L->Window != NULL)
	      {
		/* Does it belong to a GZZ window and is it
	           the outer window of that GZZ window? */
		if (0  != (((struct Window *)_L->Window)->Flags & WFLG_GIMMEZEROZERO) &&
	            _L ==  ((struct Window *)_L->Window)->BorderRPort->Layer            )
		{
	          /* simply refresh that window's frame */

		  BeginUpdate(_L);
	          RefreshWindowFrame((struct Window *)_L->Window);
		  EndUpdate(_L, TRUE);

	          _L->Flags &= ~LAYERREFRESH;
		}
		else
	          WindowNeedsRefresh((struct Window *)_L->Window,
	                             IntuitionBase);
	      }
	    }
	   _L = _L->back;

	  } /* while (NULL != _L) */

	} /* if (TRUE == CheckLayersBehind) */

	if (TRUE == CheckLayersInFront)
	{
	  /* Walk through all layers in front of including the layer L
	     and check whether a layer needs a refresh 
	  */

	  if (TRUE == CheckLayersBehind)
	    L=L->front; /* the layer L has already been checked */

	  while (NULL != L)
	  {  
	    /* Does this Layer need a refresh and does it belong
	       to a Window ?? */
	    if (0 != (L->Flags & LAYERREFRESH))
	    {
	      if (L == targetwindow->WScreen->BarLayer)
	      {
	        RenderScreenBar(targetwindow->WScreen, TRUE, IntuitionBase);
	      } else if (L->Window != NULL) {
		/* Does it belong to a GZZ window and is it
	           the outer window of that GZZ window? */
		if (0  != (((struct Window *)L->Window)->Flags & WFLG_GIMMEZEROZERO) &&
	            L  ==  ((struct Window *)L->Window)->BorderRPort->Layer            )
		{
	          /* simply refresh that window's frame */

		  BeginUpdate(L);
	          RefreshWindowFrame((struct Window *)L->Window);
		  EndUpdate(L, TRUE);

	          L->Flags &= ~LAYERREFRESH;
		}
		else
	          WindowNeedsRefresh((struct Window *)L->Window,
	                             IntuitionBase);
	      }
	    }

	    L = L->front;

	  } /* while (NULL != L) */

	} /* if (TRUE == CheckLayersInFront) */

next_action:
    	ObtainSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);	

	if (remove_am) Remove(&am->ExecMessage.mn_Node);
	if (free_am)   FreeMem(am, sizeof(struct DeferedActionMessage));

	am = next_am;
	next_am = (struct DeferedActionMessage *)am->ExecMessage.mn_Node.ln_Succ;

    	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->DeferedActionLock);
	
    } /* while (next_am) */

}
			  
			  
