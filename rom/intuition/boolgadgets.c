/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Routines for BOOL Gadgets
*/

/****************************************************************************************/

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <graphics/gfxmacros.h>

#include <string.h>

#include "intuition_intern.h"
#include "gadgets.h"

#define DEBUG_REFRESHBOOLGADGET(x)  ;

/****************************************************************************************/

#define RENDERGADGET(win,gad,rend)                  \
    if (rend)                               	    \
    {                       	    	    	    \
    	if (gad->Flags & GFLG_GADGIMAGE)    	    \
    	{                   	    	    	    \
    	    DrawImageState (rp              	    \
    	    	    	    , (struct Image *)rend  \
    	    	    	    , bbox.Left             \
    	    	    	    , bbox.Top              \
    	    	    	    , state         	    \
    	    	    	    , dri           	    \
    	    	    	   );               	    \
    	}                   	    	    	    \
    	else                	    	    	    \
    	{                   	    	    	    \
    	    DrawBorder (rp                  	    \
    	    	    	, (struct Border *)rend     \
    	    	    	, bbox.Left         	    \
    	    	    	, bbox.Top          	    \
    	    	       );                   	    \
    	}                   	    	    	    \
    }

#define GETRENDER(gad)  (gad->SelectRender && (gad->Flags & GFLG_SELECTED)) ? \
    	    	    	gad->SelectRender : gad->GadgetRender;

#define BOXSIZEX 4
#define BOXSIZEY 4

/****************************************************************************************/

static void RenderBoolLabel(struct RastPort *rp, struct Gadget *gadget, struct BBox *bbox,
                            struct DrawInfo *dri, struct IntuitionBase *IntuitionBase)
{
    #warning Amiga handmade (non-boopsi) bool gadgets do not seem to know anything about
    #warning GFLG_LABELSTRING/GFLG_LABELIMAGE. Instead they always assume GadgetText to
    #warning point to a struct IntuiText!!!

    if (gadget->GadgetText)
    {
        SetFont(rp, dri->dri_Font);

        switch (gadget->Flags & GFLG_LABELMASK)
        {
            case GFLG_LABELITEXT:
        	PrintIText (rp, gadget->GadgetText, bbox->Left, bbox->Top);
        	break;

            case GFLG_LABELSTRING:
        	{
                    STRPTR text = (STRPTR) gadget->GadgetText;
                    int    len, labelwidth, labelheight;

                    len = strlen (text);

                    labelwidth = LabelWidth (rp, text, len, IntuitionBase);
                    labelheight = rp->Font->tf_YSize;

                    SetAPen (rp, 1);
                    SetDrMd (rp, JAM1);

                    Move (rp, bbox->Left + bbox->Width  / 2 - labelwidth / 2,
                	  bbox->Top  + bbox->Height / 2 - labelheight / 2 + rp->Font->tf_Baseline);

                    RenderLabel (rp, text, len, IntuitionBase);
                    break;
        	}

            case GFLG_LABELIMAGE:
        	DrawImageState (rp, (struct Image *)gadget->GadgetText,
                        	bbox->Left,
                        	bbox->Top,
                        	IDS_NORMAL,
                        	dri);
        	break;

        } /* switch (gadget->Flags & GFLG_LABELMASK) */

    } /* GadgetText */

}

/****************************************************************************************/

void RefreshBoolGadget (struct Gadget * gadget, struct Window * window,
                        struct Requester * requester, struct IntuitionBase * IntuitionBase)
{
    struct RastPort 	*rp;
    struct GadgetInfo    gi;
    struct DrawInfo 	*dri;
    struct BBox     	 bbox;
    ULONG           	 state;
    APTR            	 render;

    DEBUG_REFRESHBOOLGADGET(dprintf("RefreshBoolGadget: gadget %p window %p req %p\n",
                                    gadget, window, requester));

    CalcBBox (window, requester, gadget, &bbox);

    state = GetGadgetState(window, gadget);

    SetupGInfo(&gi, window, requester, gadget, IntuitionBase);

    rp = ObtainGIRPort(&gi);
    if (!rp) return;

    dri = GetScreenDrawInfo(window->WScreen);

    SetDrMd (rp, JAM1);

    switch (gadget->Flags & GFLG_GADGHIGHBITS)
    {
	case GFLG_GADGHIMAGE:
            render = GETRENDER(gadget);
            RENDERGADGET(window,gadget,render);
            break;

	case GFLG_GADGHCOMP:
	case GFLG_GADGHNONE:
	case GFLG_GADGHBOX:
            render = gadget->GadgetRender;
            RENDERGADGET(window,gadget,render);
            break;

    } /* switch GadgetHighlightMethod */

    RenderBoolLabel(rp, gadget, &bbox, dri, IntuitionBase);

    if ((bbox.Width >= 1) && (bbox.Height >= 1))
    {
        switch (gadget->Flags & GFLG_GADGHIGHBITS)
        {
            case GFLG_GADGHCOMP:
        	if (gadget->Flags & GFLG_SELECTED)
        	{
                    SetDrMd (rp, COMPLEMENT);

                    RectFill (rp, bbox.Left,
                              bbox.Top,
                              bbox.Left + bbox.Width - 1,
                              bbox.Top + bbox.Height - 1);
        	}
        	break;

            case GFLG_GADGHBOX:
        	if (gadget->Flags & GFLG_SELECTED)
        	{
                    SetDrMd (rp, COMPLEMENT);

                    RectFill (rp, bbox.Left - BOXSIZEX,
                              bbox.Top  - BOXSIZEY,
                              bbox.Left + bbox.Width  - 1 + BOXSIZEX,
                              bbox.Top  + bbox.Height - 1 + BOXSIZEY);

                    RectFill (rp, bbox.Left,
                              bbox.Top,
                              bbox.Left + bbox.Width - 1,
                              bbox.Top + bbox.Height - 1);
        	}

        	break;

        } /* Highlight after contents have been drawn */

        if ( gadget->Flags & GFLG_DISABLED )
        {
            RenderDisabledPattern(rp, dri, bbox.Left,
                                  bbox.Top,
                                  bbox.Left + bbox.Width - 1,
                                  bbox.Top + bbox.Height - 1,
                                  IntuitionBase );
        }

    } /* if ((bbox.Width >= 1) && (bbox.Height >= 1)) */

    FreeScreenDrawInfo(window->WScreen, dri);

    ReleaseGIRPort(rp);

} /* RefreshBoolGadget */

/*****************************************************************************************
 
This function is called by Intuition's InputHandler when the GFLG_SELECTED state changed
 
*****************************************************************************************/

void RefreshBoolGadgetState(struct Gadget * gadget, struct Window * window,
                            struct Requester * requester, struct IntuitionBase *IntuitionBase)
{
    struct GadgetInfo    gi;
    struct RastPort 	*rp;
    struct DrawInfo 	*dri;
    struct BBox     	 bbox;
    ULONG           	 state;
    APTR            	 render;

    DEBUG_REFRESHBOOLGADGET(dprintf("RefreshBoolGadgetState: gadget %p window %p req %p\n",
                                    gadget, window, requester));

    if ((gadget->Flags & GFLG_GADGHIGHBITS) == GFLG_GADGHNONE) return;

    CalcBBox (window, requester, gadget, &bbox);

    state = GetGadgetState(window, gadget);

    SetupGInfo(&gi, window, requester, gadget, IntuitionBase);

    rp = ObtainGIRPort(&gi);
    if (!rp) return;

    dri = GetScreenDrawInfo(window->WScreen);

    SetDrMd (rp, JAM1);

    switch (gadget->Flags & GFLG_GADGHIGHBITS)
    {
	case GFLG_GADGHIMAGE:
            render = GETRENDER(gadget);
            RENDERGADGET(window,gadget,render);
            RenderBoolLabel(rp, gadget, &bbox, dri, IntuitionBase);
            break;

    } /* switch GadgetHighlightMethod */

    if ((bbox.Width >= 1) && (bbox.Height >= 1))
    {
        switch (gadget->Flags & GFLG_GADGHIGHBITS)
        {
            case GFLG_GADGHCOMP:
        	SetDrMd (rp, COMPLEMENT);

        	RectFill (rp, bbox.Left,
                	  bbox.Top,
                	  bbox.Left + bbox.Width - 1,
                	  bbox.Top + bbox.Height - 1);
        	break;

            case GFLG_GADGHBOX:
        	SetDrMd (rp, COMPLEMENT);

        	RectFill (rp, bbox.Left - BOXSIZEX,
                	  bbox.Top  - BOXSIZEY,
                	  bbox.Left + bbox.Width  - 1 + BOXSIZEX,
                	  bbox.Top  + bbox.Height - 1 + BOXSIZEY);

        	RectFill (rp, bbox.Left,
                	  bbox.Top,
                	  bbox.Left + bbox.Width - 1,
                	  bbox.Top + bbox.Height - 1);
        	break;

        } /* switch (gadget->Flags & GFLG_GADGHIGHBITS) */

    } /* if ((bbox.Width >= 1) && (bbox.Height >= 1)) */

    FreeScreenDrawInfo(window->WScreen, dri);

    ReleaseGIRPort(rp);
}

/****************************************************************************************/
