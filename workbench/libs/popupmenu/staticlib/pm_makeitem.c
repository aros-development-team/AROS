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
        struct PopupMenu *PM_MakeItem(

/*  SYNOPSIS */
        Tag tag1,
        ...)

/*  FUNCTION
        Allocate a popup-menu item and initialize its attributes.
        This is the variadic form of PM_MakeItemA().

    INPUTS
        tag1 - First item-attribute tag/value pair, terminated by
               TAG_DONE. See the PM_* item tags in <libraries/pm.h>.

    RESULT
        Pointer to the allocated item, or NULL if allocation fails.

    NOTES
        Attribute processing is delegated to PM_SetItemAttrsA().
        Release an unlinked item with PM_FreePopupMenu(); linked
        items are released when their owning menu is freed.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, struct PopupMenu *)

    retval = PM_MakeItemA(AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_MakeItem */
