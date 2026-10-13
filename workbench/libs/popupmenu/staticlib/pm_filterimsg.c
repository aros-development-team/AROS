/*
    Copyright (C) 2010-2026, The AROS Development Team.
    All rights reserved.
    
*/

#include <utility/tagitem.h>
#include <proto/alib.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/popupmenu.h>
extern struct PopupMenuBase * PopupMenuBase;

/*****************************************************************************

    NAME */
        APTR PM_FilterIMsg(

/*  SYNOPSIS */
        struct Window *w,
        struct PopupMenu *pm,
        struct IntuiMessage *im,
        Tag tag1,
        ...)

/*  FUNCTION
        Process popup-menu input events, including menu-button events
        and command-key shortcuts. Depending on the supplied tags and
        message, the function may open the popup menu.

    INPUTS
        w    - Window associated with the input event.
        pm   - Menu to search for command-key items or to open.
        im   - IntuiMessage to inspect.
        tag1 - First tag/value pair; terminate the sequence with
               TAG_DONE. PM_AutoPullDown, PM_MenuHandler and PM_RawKey
               control the relevant input paths.

    RESULT
        Selected item's user data or the result of the popup-menu
        operation. Returns NULL when no supported event is handled.

    NOTES
        This is the variadic form of PM_FilterIMsgA(). A zero-valued
        user-data result cannot by itself distinguish a selection
        from an unhandled event.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, APTR)

    retval = PM_FilterIMsgA(w, pm, im, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_FilterIMsg */
