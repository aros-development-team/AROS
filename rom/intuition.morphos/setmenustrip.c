/*
	(C) 1999 AROS - The Amiga Research OS
	$Id$
 
	Desc: Intuition function SetMenuStrip()
	Lang: English
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

	/* TODO:
	   The following things should be done in SetMenuStrip:
	   
	   * Calculate the menu box coords for each menu (that is the box that
	     is shown when this menu is active. Save the values in JazzX,JazzY,
	     BeatX and BeatY. This cannot be done for sub-menus as there are
	no Musical variables and subitem->SubItem cannot be used either
	as it could hold just two WORDs but we need 4 WORDs. JazzX = box_x1
	, JazzY = box_y1, BeatX = box_x2, BeatY = box_y2, everything relative
	to the menu coords (or menuitem coords, in case of a subitem box).

	     stegerg: DONE


	   * Construct the Amiga-key symbol in a size appropriate for the
	     font that is in use (or is this up to the application program?).
	!!! Should be done in OpenWindow() !!!

	     stegerg: DONE 


	   * The equivalent of the above for the checkmark.

	     stegerg: DONE


	   * Consistency checks(?). If ItemFill is NULL something must be wrong
	     for example if this is a selectable menu item.

	   */

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
