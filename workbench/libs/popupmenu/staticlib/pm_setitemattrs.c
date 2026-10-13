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
        LONG PM_SetItemAttrs(

/*  SYNOPSIS */
        struct PopupMenu *p,
        Tag tag1,
        ...)

/*  FUNCTION
        Apply attributes to an existing popup-menu item. This is the
        variadic form of PM_SetItemAttrsA().

    INPUTS
        p    - Popup-menu item to modify.
        tag1 - First attribute tag/value pair, terminated by TAG_DONE.
               See the PM_* item tags in <libraries/pm.h>.

    RESULT
        Count of attribute tags handled by the implementation;
        zero if p is NULL. The count is not a general success flag.

    NOTES
        Updating some attributes replaces or frees item-owned data,
        including titles, user-data strings and existing submenus.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = PM_SetItemAttrsA(p, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_SetItemAttrs */
