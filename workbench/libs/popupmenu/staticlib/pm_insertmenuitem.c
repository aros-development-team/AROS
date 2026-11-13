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
        LONG PM_InsertMenuItem(

/*  SYNOPSIS */
        struct PopupMenu *menu,
        Tag tag1,
        ...)

/*  FUNCTION
        Process insertion tags to link existing popup-menu items into
        a menu or submenu. This is the variadic form of
        PM_InsertMenuItemA().

    INPUTS
        menu - Base of the menu list into which items are inserted.
        tag1 - First tag/value pair, terminated by TAG_DONE. Select an
               insertion position with a PM_Insert_* tag, then supply
               an item pointer using PM_Insert_Item.

    RESULT
        Number of non-NULL PM_Insert_Item tags processed with a
        non-NULL insertion reference. This count does not guarantee
        that an item was actually linked into the menu.

    NOTES
        The function links supplied item nodes; it does not copy
        or allocate the items.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = PM_InsertMenuItemA(menu, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_InsertMenuItem */
