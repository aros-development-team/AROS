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
        LONG PM_LayoutMenu(

/*  SYNOPSIS */
        struct Window *window,
        struct PopupMenu *pm,
        Tag tag1,
        ...)

/*  FUNCTION
        Reserved layout entry point. The current PM_LayoutMenuA()
        implementation does not perform a layout operation.

    INPUTS
        window - Window argument, currently unused.
        pm     - Menu argument, currently unused.
        tag1   - First tag/value pair, terminated by TAG_DONE;
                 currently unused by the implementation.

    RESULT
        Always returns zero.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = PM_LayoutMenuA(window, pm, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* PM_LayoutMenu */
