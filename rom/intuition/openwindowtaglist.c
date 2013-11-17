/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/utility.h>
#include "intuition_intern.h"
#include <utility/tagitem.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH2(struct Window *, OpenWindowTagList,

/*  SYNOPSIS */
        AROS_LHA(struct NewWindow *, newWindow, A0),
        AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 101, Intuition)

/*  FUNCTION
	Open a new window.

    INPUTS
	NewWindow - structure with window specification. This is for
	            compatibility with OpenWindow() and usually set to NULL
	tagList   - tags which specify appearance and behaviour of the window

    TAGS
	WA_Left      - Left edge of the window
	WA_Top       - Top edge of the window
	WA_Width     - Width of the window
	WA_Height    - Height of the window
	WA_DetailPen - Pen number for window details (obsolete)
	WA_BlockPen  - Pen number for filled blocks (obsolete)
	WA_IDCMP     - Define what events should send messages to your task

	WA_Flags
	    Initial values for various boolean window properties. Can be
	    overwritten by WA_... tags.

	WA_Gadgets (struct Gadget *)
	    Pointer to a linked list of gadgets

	WA_Title (STRPTR) - Window title string

	WA_CustomScreen (struct Screen *)
	    Open window on the given screen

	WA_SuperBitMap (struct BitMap *)
	    Create window with superbitmap refreshing

	WA_MinWidth           - Minimum width of the window
	WA_MinHeight          - Minimum height of the window
	WA_MaxWidth           - Maximum width of the window
	WA_MaxHeight          - Maximum height of the window
	    Use 0 to keep the current size as limit. The maximums can be
	    set to -1 or ~0 to limit size only to screen dimension.

	WA_SizeGadget (BOOL)  - Make window resizeable
	WA_DragBar (BOOL)     - Make window dragable
	WA_DepthGadget (BOOL) - Add a depth gadget
	WA_CloseGadget (BOOL) - Add a close gadget

	WA_Backdrop (BOOL)
	    Create a window which is placed behind other windows

	WA_ReportMouse (BOOL) - Store mouse position in struct Window

	WA_NoCareRefresh (BOOL)
	    Use this if you don't want to be responsible for calling
	    BeginRefresh()/EndRefresh().

	WA_Borderless (BOOL) - Create borderless window

	WA_Activate (BOOL)
	    Make this window the active one, i.e. it
	    receives the input from mouse and keyboard.

	WA_RMBTrap (BOOL)
	    Set to TRUE if you want to get button events
	    events for the right mouse button.

	WA_SimpleRefresh (BOOL)
	    Enable simplerefresh mode. Only specify if TRUE.

	WA_SmartRefresh (BOOL)
	    Enable smartrefresh mode. Only specify if TRUE.

	WA_SizeBRight (BOOL)    - Place size gadget in right window border
	WA_SizeBBottom (BOOL)   - Place size gadget in bottom window border

	WA_GimmeZeroZero (BOOL)
	    Create a GimmeZeroZero window. The window borders have their own
	    layer, so you can't overdraw it. The coordinate 0,0 is related to
	    the inner area of the window. This makes handling of windows
	    easier, but it slows down the system.

	WA_NewLookMenus (BOOL)
	    Use DrawInfo colors for rendering the menu bar.

	WA_ScreenTitle (STRPTR)
	    Screen title which is shown when window is active.

	WA_AutoAdjust (BOOL)
	    TRUE means that Intuition can move or shrink the window
	    to fit on the screen, within the limits given with
	    WA_MinWidth and WA_MinHeight. This attribute defaults
	    to TRUE when you call OpenWindowTags() with a NULL pointer
	    for NewWindow.

	WA_InnerWidth
	WA_InnerHeight
	    Dimensions of the interior region of the window.

	    Note that this restricts border gadgets:
	    - GACT_LEFTBORDER gadgets can't be GFLG_RELWIDTH if
	      WA_InnerWidth is used.
	    - GACT_RIGHTBORDER gadgets must be GFLG_RELRIGHT if
	      WA_InnerWidth is used.
	    - GACT_TOPBORDER gadgets can't be GFLG_RELHEIGHT if
	      WA_InnerHeight is used.
	    - GACT_BOTTOMBORDER gadgets must be GFLG_RELBOTTOM if
	      WA_InnerHeight is used.

	WA_PubScreen (struct Screen *)
	    Open the window on the public screen with the given address.
	    An address of NULL means default public screen. You're
	    responsible that the screen stays open until OpenWindowTags()
	    has finished, i.e. 
	    you're the owner of the screen,
	    you have already a window open on the screen
	    or you use LockPubScreen()

	WA_PubScreenName (STRPTR)
	    Open the window on the public screen with the given name.

	WA_PubScreenFallBack (BOOL)
	    TRUE means that the default public screen can be used if
	    the specified named public screen is not available.

	WA_Zoom (WORD *)
	    4 WORD's define the initial Left/Top/Width/Height of the
	    alternative zoom position/dimension. This adds a zoom
	    gadget to the window. If both left and top are set to ~0
	    the window will only be resized.

	WA_MouseQueue
	    Limits the number of possible mousemove messages. Can
	    be changed with SetMouseQueue().

	WA_RptQueue
	    Limits the number of possible repeated IDCMP_RAWKEY,
	    IDCMP_VANILLAKEY and IDCMP_IDCMPUPDATE messages.

	WA_BackFill (struct Hook *)
	    Function to be called for backfilling

 	WA_MenuHelp (BOOL)
	    Enables menuhelp. Pressing the help key during menu handling
	    sends IDCMP_MENUHELP messages.

	WA_NotifyDepth (BOOL)
	    If TRUE send IDCMP_CHANGEWINDOW events when window is
	    depth arranged. Code field will be CWCODE_DEPTH.

	WA_Checkmark (struct Image *)
	    Image to use as a checkmark in menus.

	WA_AmigaKey (struct Image *)
	    Image to use as the Amiga-key symbol in menus.

	WA_Pointer (APTR)
	    The pointer to associate with the window. Use NULL
	    for the Preferences default pointer. You can create
	    custom pointers with NewObject() on "pointerclass".
	    Default: NULL.

	WA_BusyPointer (BOOL)
	    Enable the Preferences busy-pointer.
	    Default: FALSE.

	WA_PointerDelay (BOOL)
	    Set this to TRUE to delay change of the pointer image.
	    This avoids flickering of the mouse pointer when it's
	    changed for short times.

	WA_HelpGroup (ULONG)
	    Get IDCMP_GADGETHELP messages not only from the active
	    window, but from all its windows.
	    You have to get a help ID with utility.library/GetUniqueID()
	    and use it as data for WA_HelpGroup for all windows.

	WA_HelpGroupWindow (struct Window *)
	    Alternative for WA_HelpGroup. Use the helpgroup of
	    another window.

	WA_TabletMessages (BOOL)
	    Request extended tablet data.
	    Default: FALSE

	WA_ToolBox (BOOL)
	    Make this window a toolbox window

	WA_Parent (struct Window *)
	    Make the window a child of the given window.

	WA_Visible (BOOL)
	    Make window visible.
	    Default: TRUE

	WA_ShapeRegion (struct Region *)

	WA_ShapeHook (struct Hook *)

    RESULT
        A pointer to the new window or NULL if it couldn't be
        opened. Reasons for this might be lack of memory or illegal
        attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExtNewWindow  nw =
    {
        0, 0,       	/* Left, Top */
        ~0, ~0,     	/* Width, Height */
        0xFF, 0xFF, 	/* DetailPen, BlockPen */
        0L,         	/* IDCMPFlags */
        0L,         	/* Flags */
        NULL,       	/* FirstGadget */
        NULL,       	/* CheckMark */
        NULL,       	/* Title */
        NULL,       	/* Screen */
        NULL,       	/* BitMap */
        0, 0,       	/* MinWidth, MinHeight */
        0, 0,       	/* MaxWidth, MaxHeight */
        WBENCHSCREEN,   /* Type */
        NULL        	/* Extension (taglist) */
    };
    struct Window       *window;

    DEBUG_OPENWINDOWTAGLIST(dprintf("OpenWindowTagList: NewWindow 0x%lx TagList 0x%lx\n",
                                    newWindow, tagList));

    if (newWindow)
    {
        ASSERT_VALID_PTR_ROMOK(newWindow);

        CopyMem (newWindow, &nw, (newWindow->Flags & WFLG_NW_EXTENDED) ? sizeof (struct ExtNewWindow) :
                 sizeof (struct NewWindow));

        if (tagList)
        {
            ASSERT_VALID_PTR_ROMOK(tagList);

            nw.Extension = tagList;
            nw.Flags |= WFLG_NW_EXTENDED;
        }
    }
    else
    {
        struct TagItem tags[2] =
        {
            {WA_AutoAdjust  ,  TRUE 	},
            {TAG_END	    ,       0}
        };

        nw.Extension = tags;
        nw.Flags |= WFLG_NW_EXTENDED;

        if (tagList)
        {
            ASSERT_VALID_PTR_ROMOK(tagList);

            tags[1].ti_Tag = TAG_MORE;
            tags[1].ti_Data = (IPTR)tagList;
        }
    }


    window = OpenWindow ((struct NewWindow *)&nw);

    return window;

    AROS_LIBFUNC_EXIT

} /* OpenWindowTagList */
