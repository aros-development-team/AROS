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
        struct PopupMenu *PM_MakeMenu(

/*  SYNOPSIS */
        Tag tag1,
        ...)

/*  FUNCTION
        Link previously allocated menu items into a menu list.
        This is the variadic form of PM_MakeMenuA().

    INPUTS
        tag1 - First tag/value pair, terminated by TAG_DONE. Each
               PM_Item tag supplies a pointer to an existing item.

    RESULT
        Pointer to the first item of the list, or NULL when no items
        are supplied or a PM_Item tag contains a NULL pointer.

    NOTES
        Items are linked in the order supplied, not copied. If a
        PM_Item tag contains NULL, the implementation frees the items
        already linked in this call. Free a completed menu with
        PM_FreePopupMenu().

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, struct PopupMenu *)

    retval = PM_MakeMenuA(AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_MakeMenu */
