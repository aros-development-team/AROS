/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

/*****************************************************************************
 
    NAME */

AROS_LH0(IPTR, OpenWorkBench,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
    Attempt to open the Workbench screen.
 
    INPUTS
    None.
 
    RESULT
    Tries to (re)open WorkBench screen. If successful return value
    is a pointer to the screen structure, which shouldn't be used,
    because other programs may close the WorkBench and make the
    pointer invalid.
    If this function fails the return value is NULL.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    CloseWorkBench()
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *wbscreen;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: <%s>\n",
                                FindTask(NULL)->tc_Node.ln_Name));

    LockPubScreenList();

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Workbench 0x%lx\n",
                                (ULONG) wbscreen));

    if (wbscreen)
    {
        DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: returning Workbench screen at 0x%lx\n",
                                    (ULONG) wbscreen));

        UnlockPubScreenList();

#ifdef INTUITION_NOTIFY_SUPPORT
        /* Notify that the Workbench screen is open */
        /* NOTE: Original screennotify.library notify in this case, too! */
        sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif
        return (IPTR)wbscreen;
    }
    else
    {
        /* Open the Workbench screen if we don't have one. */
        
	WORD  width  = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width;
        WORD  height = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height;
        WORD  depth  = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth;
	ULONG modeid = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID;
	
        struct TagItem screenTags[] =
        {   
            { SA_Width,                0                  }, /* 0 */
            { SA_Height,               0                  }, /* 1 */
            { SA_Depth,                depth              }, /* 2 */
	    { SA_DisplayID,            0                  }, /* 3 */
            { SA_LikeWorkbench,        TRUE               }, /* 4 */
            { SA_Type,                 WBENCHSCREEN       }, /* 5 */
            { SA_Title,         (IPTR) "Workbench Screen" }, /* 6 */
            { SA_PubName,       (IPTR) "Workbench"   	  }, /* 7 */
            { SA_SharePens,            TRUE               }, /* 8 */
            { TAG_END,                 0           	  }
        };
	
	APTR disphandle = FindDisplayInfo(modeid);
	
        if (!disphandle)
	{
    	    struct TagItem modetags[] =
	    {
	        { BIDTAG_DesiredWidth,  width  },
	        { BIDTAG_DesiredHeight, height },
	        { BIDTAG_Depth,         depth  },
	        { TAG_DONE,             0  	   }
	    };
	    	                           
	    modeid     = BestModeIDA(modetags);
	    disphandle = FindDisplayInfo(modeid);
	}
	
	if (disphandle)
	{
	    struct DimensionInfo dim;

	    #define BOUND(min, val, max) \
	        (((min) > (val)) ? (min) : ((max) < (val)) ? (max) : (val))
            
	    if (GetDisplayInfoData(disphandle, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, 0))
            {
	        width  = BOUND(dim.MinRasterWidth,  width,  dim.MaxRasterWidth);
		height = BOUND(dim.MinRasterHeight, height, dim.MaxRasterHeight);
            }
	    screenTags[3].ti_Data = modeid;
        }
	else
	    screenTags[3].ti_Tag  = TAG_IGNORE;

	screenTags[0].ti_Data = width;
        screenTags[1].ti_Data = height;
        
	DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Trying to open Workbench screen\n"));

        wbscreen = OpenScreenTagList(NULL, screenTags);

        if( !wbscreen )
        {
            DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: failed to open Workbench screen !!!!\n"));

            UnlockPubScreenList();
            return 0;
        }

        GetPrivIBase(IntuitionBase)->WorkBench = wbscreen;

        /* Make the screen public. */
        PubScreenStatus( wbscreen, 0 );

    }

    /* We have opened the Workbench Screen. Now  tell the Workbench process
       to open it's windows, if there is one. We still do have the pub screen
       list locked. But while sending the Message to the Workbench task we
       must unlock the semaphore, otherwise there can be deadlocks if the
       Workbench task itself does something which locks the pub screen list.

       But if we unlock the pub screen list, then some other task could try
       to close the Workbench screen in the meantime. The trick to solve
       this problem is to increase the psn_VisitorCount of the Workbench
       screen here, before unlocking the pub screen list. This way the
       Workbench screen cannot go away. */

    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount++;
    DEBUG_VISITOR(dprintf("OpenWorkbench: new VisitorCount %ld\n",
                          GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount));

    UnlockPubScreenList();

    DEBUG_VISITOR(dprintf("OpenWorkbench: notify Workbench\n"));

    /* Don't call this function while pub screen list is locked! */
    TellWBTaskToOpenWindows(IntuitionBase);

    /* Now fix the psn_VisitorCount we have increased by one, above. It's probably
       better to do this by hand, instead of calling UnlockPubScreen, because Un-
       lockPubScreen can send signal to psn_SigTask. */

    LockPubScreenList();
    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount--;
    DEBUG_VISITOR(dprintf("OpenWorkbench: new VisitorCount %ld\n",
                          GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount));
    UnlockPubScreenList();

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Notify that the Workbench screen is open again */
    sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif

    return (IPTR)wbscreen;

    AROS_LIBFUNC_EXIT

} /* OpenWorkBench */
