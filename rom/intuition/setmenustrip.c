/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include "menus.h"

void CalculateDims(struct Window *win, struct Menu *menu);
void Characterize(struct Menu *menu);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>
#include <proto/exec.h>
#include <intuition/intuition.h>

AROS_LH2(BOOL, SetMenuStrip,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Menu   *, menu  , A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 44, Intuition)

/*  FUNCTION
    This function adds a MenuStrip to the Window, which can be invoked
    by the user after this call by pressing the right mouse button.
    Menus with no MenuItems will not be attached.
 
    INPUTS
    window - The window to add the MenuStrip to
    menu   - The menu to be added to the window above.
 
    RESULT
    TRUE if all menus have at least one menuitem.
 
    NOTES
    This function calculates internal values and is therfore the
    official way to add a new MenuStrip to Window.
    Always do a ClearMenuStrip() before closing the Window or adding
    another MenuStrip to the Window.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    ResetMenuStrip(), ClearMenuStrip()
 
    INTERNALS
 
    HISTORY
    11.06.99  SDuvan  implemented function
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    SANITY_CHECKR(window,FALSE)

#define HASSUBITEM 0x8000

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    /* If a menu is active for this task, we must wait until the
       user is done. We check the task rather than the window as
       semaphores is owned by tasks... */
    /* struct Task *me = FindTask(NULL); */

    /* This must be before CalculateDims(). */
    Characterize(menu);

    /* When entering here, this menustrip is NOT displayed as the user has
       removed it from the window using ClearMenuStrip() if it was ever
       attached to a window. */
    CalculateDims(window, menu);

#if 0 /* stegerg: ??? */

    /*
    if(me == GPB(IntuiBase)->ib_ActiveMenuTask)
{
    ObtainSemaphore(&GPB(IntuiBase)->ib_MenuWaitLock);

    AddTail((struct Node *)me, &GPB(IntuiBase)->ib_MenuWaitList);

    ReleaseSemaphore(&GPB(IntuiBase)->ib_MenuWaitLock);

    Wait(SIGF_INTUITION);
}
    */
#endif

    window->MenuStrip = menu;

#if 0 /* stegerg: ??? */
    /* Note that we have to do a similar test in the input handler
       as well. */

    /* If we were just one of the tasks in the list... */

    /*    if(me != GPB(IntuitionBase)->ib_ActiveMenuTask)
{
    struct Task *sleeper;

    ObtainSemaphore(&GPB(IntuitionBase)->ib_MenuWaitLock);
    sleeper = RemHead(&GPB(IntuitionBase)->ib_MenuWaitList);
    ReleaseSemaphore(&GPB(IntuitionBase)->ib_MenuWaitLock);

    if(sleeper)
     Signal(sleeper, SIGF_INTUITION);
}
    */
#endif

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* SetMenuStrip */

void CalculateDims(struct Window *win, struct Menu *menu)
{
    struct MenuItem *item;

    while(menu != NULL)
    {
        item = menu->FirstItem;

        GetMenuBox(win, item, &menu->JazzX, &menu->JazzY, &menu->BeatX, &menu->BeatY);

        menu = menu->NextMenu;
    }
}

/* Mark items that has subitems. This is necessary for the input handler
   code. It's not possible to check item->SubItem within it as we save
   the layer coordinates there. */
void Characterize(struct Menu *menu)
{
    while(menu != NULL)
    {
        struct MenuItem *item;

        item = menu->FirstItem;

        while(item != NULL)
        {
            if(item->SubItem != NULL)
                item->Flags |= HASSUBITEM;

            item = item->NextItem;
        }

        menu = menu->NextMenu;
    }
}
