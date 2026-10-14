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
        IPTR PM_OpenPopupMenu(

/*  SYNOPSIS */
        struct Window *prevwnd,
        Tag tag1,
        ...)

/*  FUNCTION
        Open and operate a popup menu using a window and a variable-
        length tag list. This is the variadic form of
        PM_OpenPopupMenuA().

    INPUTS
        prevwnd - Parent window; if NULL, the implementation uses the
                  active screen through an internal window surrogate.
        tag1   - First tag/value pair, terminated by TAG_DONE. PM_Menu
                 supplies the menu to display; other PM_* tags control
                 position, input and optional callbacks.

    RESULT
        User data associated with the selected item, returned as an
        IPTR. Returns zero if no result is produced or popup-menu
        initialization fails.

    NOTES
        The call runs the popup-menu interaction and forwards the
        value returned by PM_OpenPopupMenuA().

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = PM_OpenPopupMenuA(prevwnd, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_OpenPopupMenu */
