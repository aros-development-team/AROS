/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/graphics.h>
#include <proto/layers.h>
#include <string.h>

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

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    EnterFunc(bug("RefreshWindowFrame(window=%p)\n", window));

    int_refreshwindowframe(window, FALSE, IntuitionBase);
    
    ReturnVoid("RefreshWindowFrame");

    AROS_LIBFUNC_EXIT
    
} /* RefreshWindowFrame */

VOID int_refreshwindowframe(struct Window *window, BOOL onlytitle, struct IntuitionBase *IntuitionBase)
{
    /* Draw a frame around the window */
    
    struct RastPort *rp = window->BorderRPort;
    struct DrawInfo *dri;
    struct Region   *old_clipregion;
    WORD    	     old_scroll_x, old_scroll_y;
        
    if (!(window->Flags & WFLG_BORDERLESS))
    {    	
	dri = GetScreenDrawInfo(window->WScreen);

	if (dri)
	{	    
	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

	    LockLayerRom(rp->Layer);

	    old_scroll_x = rp->Layer->Scroll_X;
	    old_scroll_y = rp->Layer->Scroll_Y;
	    
	    rp->Layer->Scroll_X = 0;
	    rp->Layer->Scroll_Y = 0;
	    
	    old_clipregion = InstallClipRegion(rp->Layer, NULL);
	    
	    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
	    if (window->BorderLeft > 0) CheckRectFill(rp, 
	                                              0,  
	                                              0, 
	                                              0, 
	                                              window->Height - 1,
	                                              IntuitionBase);

	    if (window->BorderTop > 0)  CheckRectFill(rp, 
	                                              0, 
	                                              0, 
	                                              window->Width - 1, 
	                                              0,
	                                              IntuitionBase);

	    if (window->BorderRight > 1) CheckRectFill(rp, 
	                                               window->Width - window->BorderRight,
	                                               window->BorderTop,
	    					       window->Width - window->BorderRight,
	    					       window->Height - window->BorderBottom,
	    					       IntuitionBase);
	    					       
	    if (window->BorderBottom > 1) CheckRectFill(rp,
	                                                window->BorderLeft,
	                                                window->Height - window->BorderBottom,
	    					        window->Width - window->BorderRight,
	    					        window->Height - window->BorderBottom,
	    					        IntuitionBase);
	    
	    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	    if (window->BorderRight > 0) CheckRectFill(rp, 
	                                               window->Width - 1, 
	                                               1, 
	                                               window->Width - 1, 
	                                               window->Height - 1,
	                                               IntuitionBase);
	                                               
	    if (window->BorderBottom > 0) CheckRectFill(rp, 
	                                                1, 
	                                                window->Height - 1, 
	                                                window->Width - 1, 
	                                                window->Height - 1,
	                                                IntuitionBase);
	                                                
	    if (window->BorderLeft > 1) CheckRectFill(rp, 
	                                              window->BorderLeft - 1, 
	                                              window->BorderTop - 1,
	    					      window->BorderLeft - 1, 
	    					      window->Height - window->BorderBottom,
	    					      IntuitionBase);
	    					      
	    if (window->BorderTop > 1) CheckRectFill(rp, 
	                                             window->BorderLeft - 1, 
	                                             window->BorderTop - 1,
	    				       	     window->Width - window->BorderRight, 
	    				       	     window->BorderTop - 1,
	    				       	     IntuitionBase);
	    
	   
	    SetAPen(rp, dri->dri_Pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
	    if (window->BorderLeft > 2) CheckRectFill(rp,   
	                                              1, 
	                                              1, 
	                                              window->BorderLeft - 2,
	                                              window->Height - 2,
	                                              IntuitionBase);
	                                              
	    if (window->BorderTop > 2)  CheckRectFill(rp, 
	                                              1, 
	                                              1, 
	                                              window->Width - 2, 
	                                              window->BorderTop - 2,
	                                              IntuitionBase);
	                                              
	    if (window->BorderRight > 2) CheckRectFill(rp, 
	                                               window->Width - window->BorderRight + 1, 
	                                               1,
	    					       window->Width - 2, 
	    					       window->Height - 2,
	    					       IntuitionBase);
	    					       
	    if (window->BorderBottom > 2) CheckRectFill(rp, 
	                                                1, 
	                                                window->Height - window->BorderBottom + 1,
	    			   		        window->Width - 2, 
	    					        window->Height - 2,
	    					        IntuitionBase);
		
	    if (NULL != window->Title)
	    {
		int left = 0, right = window->Width - 1;
		struct Gadget *g;

		for (g = window->FirstGadget; g; g = g->NextGadget)
		{
		    if (g->Activation & GACT_TOPBORDER)
		    {
			if (g->LeftEdge >= 0)
			{
			    if (g->LeftEdge + g->Width > left)
				left = g->LeftEdge + g->Width;
			}
			else
			{
			    if (g->LeftEdge + window->Width < right)
				right = g->LeftEdge + window->Width;
			}
		    }
		}

		if (right - left > 6)
		{
		    ULONG textlen, titlelen;
		    struct TextExtent te;

		    SetFont(rp, dri->dri_Font);

		    titlelen = strlen(window->Title);
		    textlen = TextFit(rp
			    , window->Title
			    , titlelen
			    , &te
			    , NULL
			    , 1
			    , right - left - 6
			    , window->BorderTop - 2);

		    SetAPen(rp, dri->dri_Pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLTEXTPEN : TEXTPEN]);
		    SetDrMd(rp, JAM1);
		#ifdef __MORPHOS__
		    Move(rp, left + 3, dri->dri_Font->tf_Baseline + 1);
		#else
		    Move(rp, left + 3, dri->dri_Font->tf_Baseline + 2);
		#endif
		    Text(rp, window->Title, textlen);
		}
	    }
	    
	    int_refreshglist(window->FirstGadget, 
	                     window, 
	                     NULL, 
	                     -1, 
	                     REFRESHGAD_BORDER, 
	                     0, 
	                     IntuitionBase);
						  
	    InstallClipRegion(rp->Layer, old_clipregion);
	    
	    rp->Layer->Scroll_X = old_scroll_x;
	    rp->Layer->Scroll_Y = old_scroll_y;
	    	    	                     	    
	    UnlockLayerRom(rp->Layer);
	    
            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

	    FreeScreenDrawInfo(window->WScreen, dri);
	    
	} /* if (dri) */
	
    } /* if (!(win->Flags & WFLG_BORDERLESS)) */    
}
