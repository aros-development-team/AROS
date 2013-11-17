/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <proto/intuition.h>

        AROS_LH2(struct Screen *, OpenScreenTagList,

/*  SYNOPSIS */
        AROS_LHA(struct NewScreen *, newScreen, A0),
        AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 102, Intuition)

/*  FUNCTION
        Open a screen

    INPUTS
        newScreen - struct with screen specification. This is for compatibility
            with OpenScreen() and usually set to NULL.
        tagList   - tags which specify the screen

    TAGS
	SA_Left
	    Default: 0
	    
	SA_Top
	    Default: 0
	    
	SA_Width
	    Default depends on display clip

	SA_Height
	    Default depends on display clip
	    
	SA_Depth
	    Select depth of screen. This specifies how many
	    colors the screen can display.
	    Default: 1
	    
	SA_DetailPen
	    Pen number for details.
	    Default: 0
	    
	SA_BlockPen
	    Pen number for block fills.
	    Default: 1
	    
	SA_Title (STRPTR)
	    Default: NULL
	
	SA_Font (struct TextAttr *)
	    Default: NULL, meaning user's preferred monospace font
	    
	SA_BitMap (struct BitMap *)
	    Provide a custom bitmap.

	SA_ShowTitle (BOOL)
	    Default: TRUE
	
	SA_Behind (BOOL)
	    Screen will be created behind other open screens.
	    Default: FALSE
	    
	SA_Quiet (BOOL)
	    Intuition doesn't draw system gadgets and screen title.
	    Defaults: FALSE

	SA_Type
	    PUBLICSCREEN or CUSTOMSCREEN.

	SA_DisplayID
	    32-bit display mode ID, as defined in the <graphics/modeid.h>.

	SA_Overscan
	    Set an overscan mode.

	    Possible values:

	    OSCAN_TEXT - A region which is fully visible.
	    Recommended for text display.

	    OSCAN_STANDARD - A region whose edges are "just out of view."
	    Recommended for games and presentations.

	    OSCAN_MAX - Largest region which Intuition can handle comfortably.

	    OSCAN_VIDEO - Largest region the graphics.library can display.

	    Default: OSCAN_TEXT

	SA_DClip (struct Rectangle *)
	    Define a DisplayClip region. See QueryOverscan().
	    It's easier to use SA_Overscan.

	SA_AutoScroll (BOOL)
	    Screens can be larger than the DisplayClip region. Set this tag
	    to TRUE if you want to enable automatic scrolling when you reach
	    the edge of the screen with the mouse pointer.

	SA_PubName (STRPTR)
	    Make this screen a public screen with the given name.
	    Screen is opened in "private" mode.

	SA_Pens (UWORD *)
	    Define the pen array for struct DrawInfo. This enables
	    the 3D look.

	    This array contains often just the terminator ~0.
	    You define a list of pens which overwrite the DrawInfo pens.
	    The pen arrayy must be terminated with ~0.
	    
	SA_PubTask (struct Task *)
	    Task to be signalled, when last visitor window of a public
	    screen is closed.

	SA_PubSig (UBYTE)
	    Signal number used to notify a task when the last visitor window
	    of a public screen is closed.

	SA_Colors (struct ColorSpec *)
	    Screen's initial color palette. Array must be terminated
	    with ColorIndex = -1.

	SA_FullPalette (BOOL)
	    Intuition maintains a set of 32 preference colors.
	    Default: FALSE

	SA_ErrorCode (ULONG *)
	    Intuition puts additional error code in this field when
	    opening the screen failed.
	    OSERR_NOMONITOR	- monitor for display mode not available.
	    OSERR_NOCHIPS	- you need newer custom chips for display mode.
	    OSERR_NOMEM		- couldn't get normal memory
	    OSERR_NOCHIPMEM	- couldn't get chip memory
	    OSERR_PUBNOTUNIQUE	- public screen name already used
	    OSERR_UNKNOWNMODE	- don't recognize display mode requested
	    OSERR_TOODEEP	- screen too deep to be displayed on
				  this hardware (V39)
	    OSERR_ATTACHFAIL	- An illegal attachment of screens was
				  requested (V39)

	SA_SysFont
	    Select screen font type. This overwrites SA_Font.

	    Values:
		0 - Fixed-width font (old-style)
		1 - Font which is set by font preferences editor. Note:
		    windows opened on this screen will still have the rastport
		    initialized with the fixed-width font (sysfont 0).

	    Default: 0

	SA_Parent (struct Screen *)
	    Attach the screen to the given parent screen.

	SA_FrontChild (struct Screen *)
	    Attach given child screen to this screen. Child screen
	    must already be open. The child screen will go to the 
	    front of the screen group.

	SA_BackChild (struct Screen *)
	    Attach given child screen to this screen. Child screen
	    must already be open. The child screen will go behind other
	    child screens.

	SA_BackFill (struct Hook *)
	    Backfill hook (see layers.library/InstallLayerInfoHook() ).

	SA_Draggable (BOOL)
	    Make screen draggable.
	    Default: TRUE

	SA_Exclusive (BOOL)
	    Set to TRUE if the screen must not share the display with
	    other screens. The screen will not be draggable and doesn't
	    appear behind other screens, but it still is depth arrangeable.
	    Default: FALSE

	SA_SharePens (BOOL)
	    Per default, Intuition obtains the pens of a public screen with
	    PENF_EXCLUSIVE. Set this to TRUE to instruct Intuition to leave
	    the pens unallocated.
	    Default: FALSE

	SA_Colors32 (ULONG *)
	    Data is forwarded to graphics.library/LoadRGB32().  
	    Overwrites values which were set by SA_Colors.

	SA_Interleaved (BOOL)
	    Request interleaved bimap. It this fails a non-interleaved
	    bitmap will be allocated.
	    Default: FALSE

	SA_VideoControl (struct TagItem *)
	    Taglist which will be  passed to VideoControl() after the
	    screen is open.

	SA_ColorMapEntries:
	    Number of entries of the ColorMap.
	    Default: 1<<depth, but not less than 32

	SA_LikeWorkbench (BOOL)
	    Inherit depth, colors, pen-array, screen mode, etc. from
	    the Workbench screen. Individual attributes can be overridden
	    with tags.
	    Default: FALSE

 	SA_MinimizeISG (BOOL)
	    Minimize the Inter-Screen-Gap. For compatibility,

    RESULT
        Pointer to screen or NULL if opening fails.

    NOTES
        If you need a pointer to the screen's bitmap use
        Screen->RastPort.BitMap instead of &Screen->BitMap.

        If you want DOS requester to appear on your screen you have to do:
            process = FindTask(0);
            process->pr_WindowPtr = (APTR) window;
        The old value of pr->WindowPtr must be reset before you quit your
        program.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExtNewScreen ns =
    {
        0, 0, -1, -1, 1,            /* left, top, width, height, depth */
        0, 1,               	    /* DetailPen, BlockPen */
        HIRES | LACE,               /* ViewModes */
        CUSTOMSCREEN | SHOWTITLE,   /* Type */
        NULL,               	    /* Font */
        NULL,               	    /* DefaultTitle */
        NULL,               	    /* Gadgets */
        NULL,               	    /* CustomBitMap */
        NULL                	    /* Extension (taglist) */
    };

    DEBUG_OPENSCREENTAGLIST(dprintf("OpenScreenTagList: NewScreen 0x%lx Tags 0x%lx\n",
                                    newScreen, tagList));
    ns.DetailPen = GetPrivIBase(IntuitionBase)->DriPens4[DETAILPEN];
    ns.BlockPen  = GetPrivIBase(IntuitionBase)->DriPens4[BLOCKPEN];

    if (newScreen)
        CopyMem (newScreen, &ns, (newScreen->Type & NS_EXTENDED) ? sizeof (struct ExtNewScreen) :
                 sizeof (struct NewScreen));

    if (tagList)
    {
        ns.Extension = tagList;
        ns.Type |= NS_EXTENDED;
    }

#ifdef __MORPHOS__
    /* calling OpenScreen through the library vector causes a loop with cgx's patch. */
    {
        extern ULONG LIB_OpenScreen(void);
	
        REG_A0 = (LONG)&ns;
        REG_A6 = (LONG)IntuitionBase;
	
        return (struct Screen *) LIB_OpenScreen();
    }
#else
    return OpenScreen ((struct NewScreen *)&ns);
#endif

    AROS_LIBFUNC_EXIT

} /* OpenScreenTagList */
