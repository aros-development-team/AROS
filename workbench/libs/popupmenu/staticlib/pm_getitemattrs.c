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
        LONG PM_GetItemAttrs(

/*  SYNOPSIS */
        struct PopupMenu *p,
        Tag tag1,
        ...)

/*  FUNCTION
        Read attributes of a popup-menu item into caller-supplied
        result locations. This is the variadic form of
        PM_GetItemAttrsA().

    INPUTS
        p    - Popup-menu item from which to read attributes.
        tag1 - First attribute tag and output-pointer pair; terminate
               with TAG_DONE. Each pointer must address storage of the
               type required by that attribute.

    RESULT
        Number of attributes retrieved; zero if p is NULL or none
        of the requested attributes can be retrieved.

    NOTES
        Some attributes return pointers into the item rather than
        independent copies. Keep the item alive while using them.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = PM_GetItemAttrsA(p, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_GetItemAttrs */
