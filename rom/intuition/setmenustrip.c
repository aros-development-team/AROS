/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function SetMenuStrip()
    Lang: English
*/
#include "intuition_intern.h"

void CalculateDims(struct Menu *menu);
void GetBox(struct MenuItem *item, struct Menu *menu,
	    WORD *xmax, WORD *ymax);
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

#define HASSUBITEM 0x8000

    /* If a menu is active for this task, we must wait until the
       user is done. We check the task rather than the window as
       semaphores is owned by tasks... */
    struct Task *me = FindTask(NULL);

    /* This must be before CalculateDims(). */
    Characterize(menu);

    /* When entering here, this menustrip is NOT displayed as the user has
       removed it from the window using ClearMenuStrip() if it was ever
       attached to a window. */
    CalculateDims(menu);

    /*
    if(me == GPB(IntuiBase)->ib_ActiveMenuTask)
    {
	ObtainSemaphore(&GPB(IntuiBase)->ib_MenuWaitLock);
	
	AddTail((struct Node *)me, &GPB(IntuiBase)->ib_MenuWaitList);
	
	ReleaseSemaphore(&GPB(IntuiBase)->ib_MenuWaitLock);
	
	Wait(SIGF_INTUITION);
    }
    */
    
    window->MenuStrip = menu;
    
    
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
    
    return TRUE;
    
    /* TODO:
       The following things should be done in SetMenuStrip:
       
       * Calculate the layer size for each menu (that is the layer that
         is shown when this menu is active. Save the values in JazzX,JazzY,
         BeatX and BeatY. The same goes for submenus but there we use some
         hackish trickery as there are no Musical variables.

       * Construct the Amiga-key symbol in a size appropriate for the
         font that is in use (or is this up to the application program?).
	 !!! Should be done in OpenWindow() !!!

       * The equivalent of the above for the checkmark.

       * Consistency checks(?). If ItemFill is NULL something must be wrong
         for example if this is a selectable menu item.

       */

    AROS_LIBFUNC_EXIT
} /* SetMenuStrip */

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

void CalculateDims(struct Menu *menu)
{
    struct MenuItem *item;

    while(menu != NULL)
    {
	item = menu->FirstItem;

	GetBox(item, menu, &menu->BeatX, &menu->BeatY);

	menu = menu->NextMenu;
    }
}


void GetBox(struct MenuItem *item, struct Menu *menu,
	    WORD *xmax, WORD *ymax)
{
    WORD right, bottom;

    right = bottom = -0x7fff;

    while(item != NULL)
    {
	right  = max(right,  item->LeftEdge + item->Width);
	bottom = max(bottom, item->TopEdge + item->Height);
	
	if(item->SubItem != NULL)
	    /* We can do this as subitems cannot have subitems. */
	    GetBox(item->SubItem, menu, (WORD *)&item->SubItem->SubItem, 
		   ((WORD *)&item->SubItem->SubItem)+1);

	item = item->NextItem;
    }

    *xmax = right + menu->LeftEdge;
    *ymax = bottom + menu->TopEdge + menu->Height;
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
