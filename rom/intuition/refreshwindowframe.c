/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.13  2000/04/08 16:37:37  stegerg
    removed ilock variable because it is unused.

    Revision 1.12  2000/03/17 22:13:46  stegerg
    Don't LockIBase before and after int_refreshglist

    Revision 1.11  2000/03/16 14:57:53  bergers
    Pass additional parameter to CheckRectFill.

    Revision 1.10  2000/01/19 19:05:01  stegerg
    renamed debug output (was "intui_RefreshWindowFrame")

    Revision 1.9  2000/01/11 16:05:00  bergers
    Update. Move some code from intuition_driver to this directoy.

    Revision 1.8  1998/12/31 19:35:39  nlorentz
    "Implemented" function (Just calls driver)

    Revision 1.7  1998/10/20 20:08:04  nlorentz
    Fixed lots of errors due to aros_not_implemented()

    Revision 1.6  1998/10/20 16:46:03  hkiel
    Amiga Research OS

    Revision 1.5  1998/09/12 20:20:09  hkiel
    converted TODO/FIXME comments to #warnings

    Revision 1.4  1997/01/27 00:36:43  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:08  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:06:49  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

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

    /* Draw a frame around the window */
    struct RastPort *rp = window->BorderRPort;
    struct DrawInfo *dri;
    struct Region *old_clipregion;
    WORD  old_scroll_x, old_scroll_y;
    
    EnterFunc(bug("RefreshWindowFrame(window=%p)\n", window));
    
    if (!(window->Flags & WFLG_BORDERLESS))
    {    	
	dri = GetScreenDrawInfo(window->WScreen);
	if (dri)
	{
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
						  
	    InstallClipRegion(rp->Layer,old_clipregion);
	    
	    rp->Layer->Scroll_X = old_scroll_x;
	    rp->Layer->Scroll_Y = old_scroll_y;
	    
	    UnlockLayerRom(rp->Layer);
	    
    	    /* Refresh all the gadgets with GACT_???BORDER activation set */

            /* The layer must not be locked when calling RefreshGList,
	       otherwise a deadlock can happen:
	       
	       task a: ObtainGirPort: ObtainSem(GadgetLock)
	       
	       ** task switch **
	       
	       task b: LockLayer
	               refreshglist -> ObtainGIRPort : ObtainSem(GadgetLock)
		                                       must wait because locked by task a
						       
	       ** task switch **
	       
	       task a: ObtainGirPort: LockLayer
	                              must wait because layer locked by task b
				      
	       --------------------------------------------------------------------
	       = Deadlock: task a tries to lock layer which is locked by task b
	                   task b will never unlock the layer because it tries to
			   ObtainSem GadgetLock which is locked by task a.
	    
	    */
	    
	    int_refreshglist(window->FirstGadget, 
	                     window, 
	                     NULL, 
	                     -1, 
	                     REFRESHGAD_BORDER, 
	                     0, 
	                     IntuitionBase);
	                     	    

	    FreeScreenDrawInfo(window->WScreen, dri);
	    
	} /* if (dri) */
	
    } /* if (!(win->Flags & WFLG_BORDERLESS)) */
    
    ReturnVoid("RefreshWindowFrame");

    AROS_LIBFUNC_EXIT
} /* RefreshWindowFrame */
