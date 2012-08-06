/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <aros/config.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

static ULONG FindMode(ULONG width, ULONG height, ULONG depth, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    ULONG modeid;
    struct TagItem modetags[] =
    {
	{ BIDTAG_DesiredWidth,  width  }, /* 0 */
	{ BIDTAG_DesiredHeight, height }, /* 1 */
	{ BIDTAG_Depth,         depth  }, /* 2 */
	{ TAG_DONE,             0      }
    };

    /* Try to find the specified mode */
    modeid = BestModeIDA(modetags);
    D(bug("[OpenWorkbench] Size: %dx%d, depth: %d, ModeID 0x%08X\n", width, height, depth, modeid));

    if (modeid == INVALID_ID)
    {
    	/* If failed, we can have this resolution, but not this depth. Try 4 colors. */
	modetags[2].ti_Data = 2;

    	modeid = BestModeIDA(modetags);
	D(bug("[OpenWorkbench] Size: %dx%d, depth: 2, ModeID 0x%08X\n", width, height, modeid));
    }

    return modeid;
}   

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

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Screen *wbscreen;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: <%s>\n",
                                FindTask(NULL)->tc_Node.ln_Name));

    /* Intuition not up yet? */
    if (!GetPrivIBase(IntuitionBase)->DefaultPointer)
    	return FALSE;

    LockPubScreenList();

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Workbench 0x%lx\n",
                                (ULONG) wbscreen));

    if (wbscreen)
    {
        DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: returning Workbench screen at 0x%lx\n",
                                    (ULONG) wbscreen));

        UnlockPubScreenList();

        FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_AFTER_OPENWB, IntuitionBase);

        return (IPTR)wbscreen;
    }
    else
    {
        /* Open the Workbench screen if we don't have one. */

	WORD  width  = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width;
        WORD  height = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height;
        WORD  depth  = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth;
	ULONG modeid = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID;
	APTR disphandle;

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

	D(bug("[OpenWorkbench] Requested size: %dx%d, depth: %d, ModeID: 0x%08lX\n", width, height, depth, modeid));

	/* First check if the specified ModeID exists in the system */
	disphandle = FindDisplayInfo(modeid);
	if (!disphandle)
	{
	    D(bug("[OpenWorkbench] Invalid ModeID given\n"));
	    modeid = INVALID_ID;
	}

        if (modeid == INVALID_ID)
	{
	    /*
	     * We were unable to find the ModeID specified in our prefs. Need to find a replacement.
	     * First we'll try to look up a mode corresponding to user-specified width, height and depth.
	     */

	    /* Specifying -1's here causes BestModeIDA() to fail, fix up the values */
	    if (width == STDSCREENWIDTH)
	    {
	        D(bug("[OpenWorkbench] Using default width %d\n", AROS_DEFAULT_WBWIDTH));
	        width = AROS_DEFAULT_WBWIDTH;
	    }
	    if (height == STDSCREENHEIGHT)
	    {
	        D(bug("[OpenWorkbench] Using default height %d\n", AROS_DEFAULT_WBHEIGHT));
	        height = AROS_DEFAULT_WBHEIGHT;
	    }
	    if (depth == -1)
	        depth = AROS_DEFAULT_WBDEPTH;

#ifdef __mc68000
	    /* FIXME: less hacky RTG detection */
	    /* select 640x480 if we appear to have RTG hardware (instead of standard PAL/NTSC mode) */
	    if (height < 480)
    	    {
    	        modeid = BestModeID(BIDTAG_DesiredWidth, 800, BIDTAG_DesiredHeight, 600,
				    BIDTAG_Depth, 8, TAG_DONE);
 
		if (modeid != INVALID_ID)
    	    	{
    	    	    /* If we have 800x600 or better mode, we assume we have RTG hardware */
		    height = 480;
	    	}
	    }
#endif

	    modeid = FindMode(width, height, depth, IntuitionBase);
	}

	if (modeid == INVALID_ID)
	{
	    /* We don't have any modes with the specified resolution. Try SVGA default (800x600) */
	    modeid = FindMode(800, 600, depth, IntuitionBase);
	}

	if (modeid == INVALID_ID)
	{
	    /* We don't have any modes with the specified resolution. Try VGA default (640x480) */
	    modeid = FindMode(640, 480, depth, IntuitionBase);
	}

	if (modeid == INVALID_ID)
	{
	    /* We don't have any modes with the specified resolution. Try Amiga default (640x200) */
	    modeid = FindMode(640, 200, depth, IntuitionBase);
	}

	if (modeid == INVALID_ID)
	{
	    /*
	     * There's no even 640x200. Perhaps we are on some mobile device.
	     * Here we try the smallest known screen. This size was picked up
	     * from configure's defaults for old PalmPilot port.
	     */
	    modeid = FindMode(160, 160, depth, IntuitionBase);
	}

	if (modeid != INVALID_ID)
	{
	    struct DimensionInfo dim;

	    #define BOUND(min, val, max) \
	        (((val) == -1) ? -1 : ((min) > (val)) ? (min) : ((max) < (val)) ? (max) : (val))

	    /* Now fix up our specified size to fit into mode's limits. */
	    if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, modeid))
            {
	        D(bug("[OpenWorkbench] Minimum size: %dx%d\n", dim.MinRasterWidth, dim.MinRasterHeight));
		D(bug("[OpenWorkbench] Maximum size: %dx%d\n", dim.MaxRasterWidth, dim.MaxRasterHeight));
		D(bug("[OpenWorkbench] Maximum depth: %d\n", dim.MaxDepth));

	        width  = BOUND(dim.MinRasterWidth,  width,  dim.MaxRasterWidth);
		height = BOUND(dim.MinRasterHeight, height, dim.MaxRasterHeight);
		depth  = BOUND(0, depth, dim.MaxDepth);
		D(bug("[OpenWorkbench] Corrected size: %dx%d %dbpp\n", width, height, depth));

		GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width  = width;
		GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height = height;
		GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth  = depth;
            }

	    /*
	     * Remember this ModeID because OpenScreen() with SA_LikeWorkbench set to TRUE
	     * looks at this field. We MUST have something valid here.
	     */
	    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID = modeid;

	    screenTags[0].ti_Data = width;
            screenTags[1].ti_Data = height;
            screenTags[2].ti_Data = depth;
	    screenTags[3].ti_Data = modeid;

	    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Trying to open Workbench screen\n"));

            FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_BEFORE_OPENWB, IntuitionBase);

            wbscreen = OpenScreenTagList(NULL, screenTags);
        }

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

    FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_AFTER_OPENWB, IntuitionBase);

    return (IPTR)wbscreen;

    AROS_LIBFUNC_EXIT

} /* OpenWorkBench */
