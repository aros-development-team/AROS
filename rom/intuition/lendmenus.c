/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, LendMenus,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, fromwindow, A0),
         AROS_LHA(struct Window *, towindow, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 134, Intuition)

/*  FUNCTION
    This function 'lends' the menus of one window to another.
    This makes the menu events (eg. menu button press) take place
    in another window's menu (ie. the other window's strip and screen).
    This function is used to unify two windows on different attached
    screens. (Eg. a painting program with an attached palette screen
    can open the menu on the main screen if the menu button is
    pressed on the palette screen.
 
    INPUTS
    fromwindow - This window's menu events will go to another window.
    towindow - This is the window that will react on the menu actions
        of the other window. If NULL 'lending' will be turned off.
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    SetMenuStrip(), ClearMenuStrip()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ASSERT_VALID_PTR(fromwindow);
    ASSERT_VALID_PTR_OR_NULL(towindow);

    DEBUG_LENDMENUS(dprintf("LendMenus(Window 0x%lx ToWindow 0x%lx)\n",fromwindow,towindow));

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    SANITY_CHECK(fromwindow);

    ((struct IntWindow *)fromwindow)->menulendwindow = towindow;

    AROS_LIBFUNC_EXIT
} /* LendMenus */
