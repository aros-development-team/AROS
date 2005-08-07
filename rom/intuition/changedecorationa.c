/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id: sendintuimessage.c 23476 2005-07-31 17:07:04Z stegerg $
*/

#include <proto/utility.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/screens.h>

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ChangeDecorationActionMsg
{
    struct IntuiActionMsg    msg;
    ULONG   	    	     which;
    Object  	    	    **decorobjptr;
    struct SignalSemaphore  *decorsem;
    struct DrawInfo  	    *dri;
};

static VOID int_changedecoration(struct ChangeDecorationActionMsg *msg,
                                 struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH4(Object *, ChangeDecorationA,

         /*  SYNOPSIS */
	 AROS_LHA(ULONG, which, D0),
	 AROS_LHA(CONST_STRPTR, classID, A1),
         AROS_LHA(struct DrawInfo *, dri, A0),
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
    struct TagItem  decor_tags[] =
    {
    	{TAG_IGNORE , (IPTR)dri 	    	    	    	    },
	{TAG_IGNORE , (IPTR)((struct IntDrawInfo *)dri)->dri_Screen },
	{TAG_DONE   	    	    	    	    	    	    }
    };
    struct ChangeDecorationActionMsg msg;
    
    ASSERT_VALID_PTR(dri);

    switch(which)
    {
    	case DECORATION_WINDOW:
	    decor_tags[0].ti_Tag = WDA_DrawInfo;
	    decor_tags[1].ti_Tag = WDA_Screen;
	    msg.decorobjptr = &((struct IntDrawInfo *)dri)->dri_WinDecorObj;
	    msg.decorsem = &((struct IntDrawInfo *)dri)->dri_WinDecorSem;
	    break;

    	case DECORATION_SCREEN:
	    decor_tags[0].ti_Tag = SDA_DrawInfo;
	    decor_tags[1].ti_Tag = SDA_Screen;
	    msg.decorobjptr = &((struct IntDrawInfo *)dri)->dri_ScrDecorObj;
	    msg.decorsem = &((struct IntDrawInfo *)dri)->dri_ScrDecorSem;
	    break;
	    
	default:
	    return 0;
    }
    
    new = NewObjectA(NULL, (UBYTE *)classID, decor_tags);
    if (new)
    {
    	ObtainSemaphore(msg.decorsem);
        old = *msg.decorobjptr;
    	*msg.decorobjptr = new;
    	ReleaseSemaphore(msg.decorsem);
    }

    msg.dri = dri;
    msg.which = which;
    DoASyncAction((APTR)int_changedecoration, &msg.msg, sizeof(msg), IntuitionBase);
    
    return old;
    
    AROS_LIBFUNC_EXIT
}

static VOID int_changedecoration(struct ChangeDecorationActionMsg *msg,
                                 struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr = ((struct IntDrawInfo *)msg->dri)->dri_Screen;
    struct Window *win;

    if (!ResourceExisting(scr, RESOURCE_SCREEN, IntuitionBase)) return;
        
    switch(msg->which)
    {
    	case DECORATION_WINDOW:	
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
		
	    } /* for(win = scr->FirstWindow; win; win = win->NextWindow) */
	    break;
	    
	case DECORATION_SCREEN:
    	    RenderScreenBar(scr, FALSE, IntuitionBase);
	    break;
	    
    } /* switch(msg->which) */
}

