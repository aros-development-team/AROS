/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id: sendintuimessage.c 23476 2005-07-31 17:07:04Z stegerg $
*/

#include <proto/utility.h>
#include <intuition/windecorclass.h>
#include <intuition/screens.h>

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ChangeWindowDecorationActionMsg
{
    struct IntuiActionMsg    msg;
    struct DrawInfo  	    *dri;
};

static VOID int_changewindowdecoration(struct ChangeWindowDecorationActionMsg *msg,
                                       struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH3(Object *, ChangeWindowDecorationA,

         /*  SYNOPSIS */
         AROS_LHA(struct DrawInfo *, dri, A0),
	 AROS_LHA(CONST_STRPTR, classID, A1),
         AROS_LHA(struct TagItem *, tagList, A2),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 153, Intuition)

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

    Object  	    *old = 0, *new;
    struct TagItem  windecor_tags[] =
    {
    	{WDA_DrawInfo	, (IPTR)dri 	    	    	    	    	},
	{WDA_Screen 	, (IPTR)((struct IntDrawInfo *)dri)->dri_Screen },
	{TAG_DONE   	    	    	    	    	    	    	}
    };
    struct ChangeWindowDecorationActionMsg msg;
    
    ASSERT_VALID_PTR(dri);

    new = NewObjectA(NULL, (UBYTE *)classID, windecor_tags);
    if (new)
    {
        old = ((struct IntDrawInfo *)dri)->dri_WinDecorObj;
    	((struct IntDrawInfo *)dri)->dri_WinDecorObj = new;
    }

    msg.dri = dri;
    DoASyncAction((APTR)int_changewindowdecoration, &msg.msg, sizeof(msg), IntuitionBase);
    
    return old;
    
    AROS_LIBFUNC_EXIT
}

static VOID int_changewindowdecoration(struct ChangeWindowDecorationActionMsg *msg,
                                       struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr = ((struct IntDrawInfo *)msg->dri)->dri_Screen;
    struct Window *win;
    
    for(win = scr->FirstWindow; win; win = win->NextWindow)
    {
    	LOCKGADGET
	
    	if (win->FirstGadget)
	{
    	    struct wdpLayoutBorderGadgets layoutmsg;

	    layoutmsg.MethodID    = WDM_LAYOUT_BORDERGADGETS;
	    layoutmsg.wdp_Window  = win;
	    layoutmsg.wdp_Gadgets = win->FirstGadget;
	    layoutmsg.wdp_Flags   = WDF_LBG_MULTIPLE |
	    	    	      	    WDF_LBG_INGADLIST;

	    LOCKSHARED_WINDECOR(msg->dri);
	    DoMethodA(((struct IntDrawInfo *)msg->dri)->dri_WinDecorObj, (Msg)&layoutmsg);	
	    UNLOCK_WINDECOR(msg->dri);
	}
	
	UNLOCKGADGET
	
    	RefreshWindowFrame(win);
    }  
}

