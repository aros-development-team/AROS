/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Add a dropzone to an AppWindow's list of AppWindowDropZones.
*/

#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/utility.h>

#include "workbench_intern.h"

/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

        AROS_LH4(struct AppWindowDropZone *, AddAppWindowDropZoneA,

/*  SYNOPSIS */
        AROS_LHA(struct AppWindow *, aw      , A0),
        AROS_LHA(ULONG             , id      , D0),
        AROS_LHA(ULONG             , userdata, D1),
        AROS_LHA(struct TagItem *  , tags    , A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 19, Workbench)

/*  FUNCTION

    A regular AppWindow, when created with AddAppWindowA() will respond to
    dropping icons anywhere in the window. With this function you can specify
    which parts of the window are suitable for dropping icons on.

    INPUTS

    aw        --  An AppWindow structure as returned by AddAppWindowA()
    id        --  drop zone identifier; for your convenience (ignored by
                  workbench.library)
    taglist   --  tags (see below)

    TAGS

    WBDZA_Left (WORD)
    Left edge of the drop zone relative to the left window edge.
    
    WBDZA_RelRight (WORD)
    Left edge of the drop zone relative to the right window edge. A value
    of -20 would create a zone located 20 pixels to the left of the right
    window edge.

    WBDZA_Top (WORD)
    Top edge of the drop zone relative to the top of the window.

    WBDZA_RelBottom (WORD)
    Top edge of the drop zone relative to the window height; a value of -20
    would create a zone located 20 pixels above the window bottom edge.

    WBDZA_Width (WORD)
    Widthof the drop zone in pixels.

    WBDZA_RelWidth (WORD)
    Width of the drop zone relative to the width of the window; a value of
    -20 would create a zone that is 20 pixels narrower than the window.

    WBDZA_Height (WORD)
    Height of the drop zone in pixels.

    WBDZA_RelHeight
    Height of the drop zone relative to the height of the window; a value of
    -20 would create a zone that is 20 pixels smaller than the window.

    WBDZA_Box (struct IBox *)
    Position and size of the drop zone

    WBDZA_Hook (struct Hook *)
    Pointer to a hook that will be called whenever the mouse enters or leaves
    your drop zone. The hook will be called with the following parameters.

         hookFunc(hook, reserved, arm);

    where the 'hookFunc' is prototyped as:

         LONG hookFunc(struct Hook *hook, APTR reserved,
	               struct AppWindowDropZoneMsg *adzm);

    Your hook function should always return 0. You must limit the rendering
    done in the 'hookFunc' to simple graphics.library operations as otherwise
    you risk deadlocking the system.

    RESULT

    A drop zone identifier or NULL if the drop zone could not be created.

    NOTES

    When a drop zone is installed, the messages received when icons are
    dropped are of type 'AMTYPE_APPWINDOWZONE' instead of 'AMTYPE_APPWINDOW'.
        You must be able to handle both types of messages if you call this
    function.
        Drop zones must be created with a position and a size; otherwise this
    function will fail.
        When an icon is dropped on a drop zone, the AppMessage am_MouseX and
    am_MouseY members will be relative to the window top left corner and NOT
    relative to the drop zone coordinates.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    This crap with dropzones should be straightened out. Only ONE type of
    message should be sent! Question is if there is a backwards compatible
    solution.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct TagItem           *tagState = tags;
    const struct TagItem           *tag;

    struct AppWindowDropZone *dropZone;

    dropZone = AllocVec(sizeof(struct AppWindowDropZone), MEMF_CLEAR);

    if (dropZone == NULL)
    {
        return NULL;
    }
    
    dropZone->awdz_ID       = id;
    dropZone->awdz_UserData = userdata;
    
    while ((tag = NextTagItem(&tagState)))
    {
        switch (tag->ti_Tag)
	{
	case WBDZA_Left:
	    dropZone->awdz_leftSpecifier = AWDZFlag_fix;
	    dropZone->awdz_Box.Left = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_RelRight:
	    dropZone->awdz_leftSpecifier = AWDZFlag_relRight;
	    dropZone->awdz_Box.Left = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_Top:
	    dropZone->awdz_topSpecifier = AWDZFlag_fix;
	    dropZone->awdz_Box.Top = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_RelBottom:
	    dropZone->awdz_topSpecifier = AWDZFlag_relBottom;
	    dropZone->awdz_Box.Top = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_Width:
	    dropZone->awdz_widthSpecifier = AWDZFlag_fix;
	    dropZone->awdz_Box.Width = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_RelWidth:
	    dropZone->awdz_widthSpecifier = AWDZFlag_relWidth;
	    dropZone->awdz_Box.Width = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_Height:
	    dropZone->awdz_heightSpecifier = AWDZFlag_fix;
	    dropZone->awdz_Box.Height = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_RelHeight:
	    dropZone->awdz_heightSpecifier = AWDZFlag_relHeight;
	    dropZone->awdz_Box.Height = (WORD)tag->ti_Data;
	    break;
	    
	case WBDZA_Box:
	    dropZone->awdz_leftSpecifier = AWDZFlag_fix;
	    dropZone->awdz_topSpecifier = AWDZFlag_fix;
	    dropZone->awdz_widthSpecifier = AWDZFlag_fix;
	    dropZone->awdz_heightSpecifier = AWDZFlag_fix;

	    if (tag->ti_Data != (IPTR)NULL)
	    {
		dropZone->awdz_Box = *(struct IBox *)tag->ti_Data;
	    }

	    break;
	    
	case WBDZA_Hook:
	    if (tag->ti_Data != (IPTR)NULL)
	    {
		dropZone->awdz_Hook = (struct Hook *)tag->ti_Data;
	    }

	    break;
        }
    }

    /* Could use a local semaphore here... */
    LockWorkbench();
    AddTail(&aw->aw_DropZones, (struct Node *)dropZone);
    UnlockWorkbench();
    
    /* NotifyWorkbench(WBNOTIFY_Create, WBNOTIFY_DropZone, WorkbenchBase); */
    
    return dropZone;

    AROS_LIBFUNC_EXIT
} /* AddAppWindowDropZoneA */

