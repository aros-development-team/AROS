/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <intuition/gadgetclass.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "intuition_customize.h"
#include "renderwindowframe.h"
#include "mosmisc.h"

#include <string.h>

#define GADGETCLIPPING

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
    struct Layer *layer = (BLAYER(window)) ? BLAYER(window) : WLAYER(window);
    struct IntDrawInfo  *dri;
    struct Region   *old_clipregion;
#ifdef GADGETCLIPPING
    struct Region   *gadgetclipregion;
#endif
    struct windowclassprefs *wcprefs=NULL;
    WORD        old_scroll_x, old_scroll_y;
    WORD left = 0;
    WORD leftoffset=0,topoffset=0,rightoffset=1,bottomoffset=1;

    if (!(window->Flags & WFLG_BORDERLESS))
    {

        dri = (struct IntDrawInfo *)GetScreenDrawInfo(window->WScreen);

        if (dri)
        {
            wcprefs = (struct windowclassprefs *)int_GetCustomPrefs(TYPE_WINDOWCLASS,dri,IntuitionBase);

            LOCK_REFRESH(window->WScreen);
            LOCKGADGET
#if 0
            if ((rp->Layer==NULL) ||
                    ((!(window->Flags & WFLG_GIMMEZEROZERO)) && (rp->Layer != window->RPort->Layer)))
            {
                dprintf("RefreshWindowFrame: Window 0x%lx\n", (ULONG) window);
                dprintf("RefreshWindowFrame: WLayer 0x%lx\n", (ULONG) window->WLayer);
                dprintf("RefreshWindowFrame: RPort 0x%lx BorderRPort 0x%lx\n", (ULONG) window->RPort, (ULONG) window->BorderRPort);
                dprintf("RefreshWindowFrame: RPort's layer 0x%lx BorderRPort's layer 0x%lx\n", (ULONG) window->RPort->Layer, (ULONG) window->BorderRPort->Layer);
            }

#endif

            if (!rp->Layer || !layer)
            {
                //must NOT happen!
                dprintf("RefreshWindowFrame: Panic! Window 0x%lx has no layer in BorderRPort!\n",(ULONG)window);
                return;
            }

            LockLayer(0,layer);

            old_scroll_x = layer->Scroll_X;
            old_scroll_y = layer->Scroll_Y;

            layer->Scroll_X = 0;
            layer->Scroll_Y = 0;

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

            old_clipregion = InstallClipRegion(layer, gadgetclipregion);
#else
            old_clipregion = InstallClipRegion(layer, NULL);
#endif

/*            if (((wcprefs->flags & WINDOWCLASS_PREFS_USEBORDERSEFFECT) && (dri->dri_Flags & DRIF_DIRECTCOLOR)) || (wcprefs->flags & WINDOWCLASS_PREFS_FRAMELESS))
            {
                leftoffset = 0;
                rightoffset = 1;
                topoffset = 0;
                bottomoffset = 1;
            }
*/
            if (!(mustbe & REFRESHGAD_TOPBORDER))
            {
                if (window->BorderLeft > leftoffset + 1)
                    RenderWindowFrame(window,leftoffset,window->BorderTop,window->BorderLeft - (leftoffset*2),window->Height - window->BorderTop - window->BorderBottom + 1,TRUE,mode,NULL,dri,IntuitionBase);

                if (window->BorderRight > rightoffset)
                    RenderWindowFrame(window,window->Width - window->BorderRight + leftoffset,window->BorderTop,window->BorderRight - rightoffset - leftoffset + 1,window->Height - bottomoffset - window->BorderTop - 1,TRUE,mode,NULL,dri,IntuitionBase);

                if (window->BorderBottom > bottomoffset)
                    RenderWindowFrame(window,leftoffset,window->Height - window->BorderBottom + topoffset,window->Width - leftoffset - rightoffset + 1,window->BorderBottom - topoffset - bottomoffset + 1,TRUE,mode,NULL,dri,IntuitionBase);
            }


            if (window->BorderTop > leftoffset*2)
            {
                ((struct IntWindow *)(window))->titlepos = left + 3;
                RenderWindowFrame(window,0,0,window->Width,window->BorderTop,TRUE,mode,NULL,(struct IntDrawInfo *)dri,IntuitionBase);
            }

#ifdef GADGETCLIPPING
            InstallClipRegion(layer,NULL);
#endif

            /* Emm: RefreshWindowFrame() is documented to refresh *all* the gadgets,
             * but when a window is activated/deactivated, only border gadgets
             * are refreshed. */
#if 1
            /* Refresh rel gadgets first, since wizard.library (burn in hell!) seems
             * to rely on that. */
            /* jDc: | ((window->Height <= window->BorderTop) ? REFRESHGAD_TOPBORDER : 0)
            ** is here to protect from sizegadget drawing on depthgadget when window is
            ** zoomed to titlebar height (example: magellan listers)
            */
            int_refreshglist(window->FirstGadget,
                             window,
                             NULL,
                             -1,
                             mustbe | REFRESHGAD_REL | (((window->Height <= window->BorderTop) ? REFRESHGAD_TOPBORDER : 0)),
                             mustnotbe,
                             IntuitionBase);
            int_refreshglist(window->FirstGadget,
                             window,
                             NULL,
                             -1,
                             mustbe | (((window->Height <= window->BorderTop) ? REFRESHGAD_TOPBORDER : 0)),
                             mustnotbe | REFRESHGAD_REL,
                             IntuitionBase);
#else
int_refreshglist(window->FirstGadget,
            window,
            NULL,
            -1,
            mustbe,
            mustnotbe ,
            IntuitionBase);
#endif

            InstallClipRegion(layer,old_clipregion);

#ifdef GADGETCLIPPING
            if (gadgetclipregion) DisposeRegion(gadgetclipregion);
#endif

            layer->Scroll_X = old_scroll_x;
            layer->Scroll_Y = old_scroll_y;

            UnlockLayer(layer);

            UNLOCKGADGET

            UNLOCK_REFRESH(window->WScreen);
            
            int_FreeCustomPrefs(TYPE_WINDOWCLASS,(struct IntDrawInfo*)dri,IntuitionBase);

            FreeScreenDrawInfo(window->WScreen, (struct DrawInfo *)dri);



        } /* if (dri) */

    } /* if (!(win->Flags & WFLG_BORDERLESS)) */
}
