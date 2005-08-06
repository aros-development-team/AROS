/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <intuition/windecorclass.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <string.h>

#include "inputhandler_actions.h"
#include "intuition_intern.h"

//#define GADGETCLIPPING

#ifdef GADGETCLIPPING
void clipbordergadgets(struct Region *region,struct Window *w,struct IntuitionBase *IntuitionBase);
#endif

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(void, RefreshWindowFrame,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 76, Intuition)

/*  FUNCTION
 
    INPUTS
 
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

    EnterFunc(bug("RefreshWindowFrame(window=%p)\n", window));

    int_refreshwindowframe(window, 0, 0, IntuitionBase);

    ReturnVoid("RefreshWindowFrame");

    AROS_LIBFUNC_EXIT
} /* RefreshWindowFrame */

VOID int_RefreshWindowFrame(struct Window *window,
                            LONG mustbe, LONG mustnotbe, LONG mode,
                            struct IntuitionBase *IntuitionBase)
{
    /* Draw a frame around the window */
    struct RastPort *rp = window->BorderRPort;
    struct DrawInfo *dri;
    struct Region   *old_clipregion;

#ifdef GADGETCLIPPING
    struct Region   *gadgetclipregion;
#endif

    WORD             old_scroll_x, old_scroll_y;

    if (!(window->Flags & WFLG_BORDERLESS))
    {
        dri = GetScreenDrawInfo(window->WScreen);
        if (dri)
        {
            LOCK_REFRESH(window->WScreen);
            LOCKGADGET
    	#if 1
            if ((rp->Layer==NULL) ||
                    ((!(window->Flags & WFLG_GIMMEZEROZERO)) && (rp->Layer != window->RPort->Layer)))
            {
                dprintf("RefreshWindowFrame: Window 0x%lx\n",window);
                dprintf("RefreshWindowFrame: WLayer 0x%lx\n",window->WLayer);
                dprintf("RefreshWindowFrame: RPort 0x%lx BorderRPort 0x%lx\n",window->RPort,window->BorderRPort);
                dprintf("RefreshWindowFrame: RPort's layer 0x%lx BorderRPort's layer 0x%lx\n",window->RPort,window->RPort->Layer,window->BorderRPort,window->BorderRPort->Layer);
            }

    	#endif

            LockLayer(0,rp->Layer);

            old_scroll_x = rp->Layer->Scroll_X;
            old_scroll_y = rp->Layer->Scroll_Y;

            rp->Layer->Scroll_X = 0;
            rp->Layer->Scroll_Y = 0;

    	#ifdef GADGETCLIPPING
            gadgetclipregion = NewRegion();
            if (gadgetclipregion)
            {
                struct Rectangle rect;

                /* add all gadgets to region */
                clipbordergadgets(gadgetclipregion,window,IntuitionBase);

                /* then remove them with xor */
                rect.MinX = 0;
                rect.MinY = 0;
                rect.MaxX = window->Width - 1;
                rect.MaxY = window->Height - 1;
                XorRectRegion(gadgetclipregion,&rect);

            }

            old_clipregion = InstallClipRegion(rp->Layer, gadgetclipregion);
    	#else
    	    old_clipregion = InstallClipRegion(rp->Layer, NULL);
    	#endif

    	    LOCKSHARED_WINDECOR(dri);
	    
    	    {
    		struct wdpDrawWinBorder  msg;

		msg.MethodID 	    	= WDM_DRAW_WINBORDER;
		msg.wdp_Window 	    	= window;
		msg.wdp_RPort     	= rp;
    	    	msg.wdp_Flags	    	= (mustbe == REFRESHGAD_TOPBORDER) ? WDF_DWB_TOP_ONLY : 0;
		
		DoMethodA(((struct IntDrawInfo *)(dri))->dri_WinDecorObj, (Msg)&msg);	
    	    }
	    
            /* Render the titlebar */
            if (NULL != window->Title)
            {
    		struct wdpDrawWinTitle  msg;

		msg.MethodID 	    	= WDM_DRAW_WINTITLE;
		msg.wdp_Window 	    	= window;
		msg.wdp_RPort     	= rp;
    	    	msg.wdp_TitleAlign  	= WD_DWTA_LEFT;
		msg.wdp_Flags	    	= 0;
		
		DoMethodA(((struct IntDrawInfo *)(dri))->dri_WinDecorObj, (Msg)&msg);	
    	    }

	    UNLOCK_WINDECOR(dri);
	    
    	#ifdef GADGETCLIPPING
            InstallClipRegion(rp->Layer,NULL);
    	#endif

            /* Emm: RefreshWindowFrame() is documented to refresh *all* the gadgets,
             * but when a window is activated/deactivated, only border gadgets
             * are refreshed. */
    	#if 1
            /* Refresh rel gadgets first, since wizard.library (burn in hell!) seems
             * to rely on that. */
            int_refreshglist(window->FirstGadget,
                             window,
                             NULL,
                             -1,
                             mustbe | REFRESHGAD_REL,
                             mustnotbe,
                             IntuitionBase);
            int_refreshglist(window->FirstGadget,
                             window,
                             NULL,
                             -1,
                             mustbe,
                             mustnotbe | REFRESHGAD_REL,
                             IntuitionBase);
    	#else
	    int_refreshglist(window->FirstGadget,
        		     window,
        		     NULL,
        		     -1,
        		     mustbe,
        		     mustnotbe,
        		     IntuitionBase);
    	#endif

            InstallClipRegion(rp->Layer,old_clipregion);

    	#ifdef GADGETCLIPPING
            if (gadgetclipregion) DisposeRegion(gadgetclipregion);
    	#endif

            rp->Layer->Scroll_X = old_scroll_x;
            rp->Layer->Scroll_Y = old_scroll_y;

            UnlockLayer(rp->Layer);
            UNLOCKGADGET

            UNLOCK_REFRESH(window->WScreen);

            FreeScreenDrawInfo(window->WScreen, (struct DrawInfo *)dri);

        } /* if (dri) */

    } /* if (!(win->Flags & WFLG_BORDERLESS)) */
}

#ifdef GADGETCLIPPING
void clipbordergadgets(struct Region *region,struct Window *w,struct IntuitionBase *IntuitionBase)
{
    struct Gadget *gad;

    for (gad = w->FirstGadget; gad; gad = gad->NextGadget)
    {
        BOOL qualified = FALSE;
        WORD left,top,right,bottom;

        top = gad->TopEdge;
    2   left = gad->LeftEdge;

        if (gad->Flags & GFLG_RELBOTTOM) top = w->Height - 1 + gad->TopEdge;
        if (gad->Flags & GFLG_RELRIGHT) left = w->Width - 1 + gad->LeftEdge;

        /* we need to be prepared for GFLG_GADGIMAGE and IA_Left set to -1, etc */
        if (gad->Flags & GFLG_GADGIMAGE && gad->SelectRender)
            left += ((struct Image *)gad->SelectRender)->LeftEdge;

        right = left + gad->Width - 1;
        bottom = top + gad->Height - 1;

        /* let's do some clipping now */

        if (left >= w->Width) continue;
        if (top >= w->Height) continue;
        if (right < 0) continue;
        if (bottom < 0) continue;

        if (left < 0) left = 0;
        if (top < 0) top = 0;
        if (right > w->Width) right = w->Width;
        if (top > w->Height) top = w->Height;

        /* sanity check */

        if (right < left) continue;
        if (bottom < top) continue;

        /* clip this gadget ? */

        if (top >= w->Height - 1 - w->BorderBottom) qualified = TRUE;
        if (left >= w->Width - 1 - w->BorderRight) qualified = TRUE;
        if (top + gad->Height - 1 <= w->BorderTop) qualified = TRUE;
        if (left + gad->Width - 1 <= w->BorderLeft) qualified = TRUE;

        if (qualified)
        {
            struct Rectangle rect;
	    
            rect.MinX = left;
            rect.MinY = top;
            rect.MaxX = right;
            rect.MaxY = bottom;

            OrRectRegion(region,&rect);

        }

    }
};
#endif

