/*
	(C) 1995-96 AROS - The Amiga Research OS
	$Id$
 
	Desc:
	Lang: english
*/
#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "renderwindowframe.h"

#include <string.h>

struct SetWindowTitlesActionMsg
{
	struct IntuiActionMsg msg;
	struct Window *window;
	UBYTE * windowTitle;
	UBYTE * screenTitle;
};

static VOID int_setwindowtitles(struct SetWindowTitlesActionMsg *msg,
							   struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
	NAME */
#include <proto/intuition.h>

AROS_LH3(void, SetWindowTitles,

		 /*  SYNOPSIS */
		 AROS_LHA(struct Window *, window, A0),
		 AROS_LHA(UBYTE         *, windowTitle, A1),
		 AROS_LHA(UBYTE         *, screenTitle, A2),

		 /*  LOCATION */
		 struct IntuitionBase *, IntuitionBase, 46, Intuition)

/*  FUNCTION
	Changes the current window and/or the screen title.
 
	INPUTS
	window - Change the title for this window or the screen which the
		window contains.
	windowTitle - New title for the window or ((UBYTE *)~0L) to keep the
		old title or NULL for no title. If you specify a string,
		this string is *NOT* copied.
	screenTitle - New title for the screen of the window or ((UBYTE *)~0L)
		to keep the old title or NULL for no title. If you specify
		a title for the screen, this title will be shown when the
		window becomes active. If you specify a string, this string
		is *NOT* copied.
 
	RESULT
	None.
 
	NOTES
	You should be careful with specifying a screen title because that
	may irritate the user.
 
	EXAMPLE
 
	BUGS
 
	SEE ALSO
 
	INTERNALS
 
	HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	struct SetWindowTitlesActionMsg msg;

	SANITY_CHECK(window)

	msg.window = window;
	msg.windowTitle = windowTitle;
	msg.screenTitle = screenTitle;

	DoASyncAction((APTR)int_setwindowtitles , &msg.msg, sizeof(msg), IntuitionBase);

	AROS_LIBFUNC_EXIT
} /* SetWindowTitles */

#define IW(x) ((struct IntWindow *)(x))

static VOID int_setwindowtitles(struct SetWindowTitlesActionMsg *msg,
							   struct IntuitionBase *IntuitionBase)
{
	struct Window *window = msg->window;
	UBYTE * windowTitle = msg->windowTitle;
	UBYTE * screenTitle = msg->screenTitle;
	BOOL change = TRUE;

	LOCKWINDOWLAYERS(window);

	if (windowTitle == (UBYTE *)~0L)
	{
		 change = FALSE;
	} else {
		window->Title = windowTitle;
		if (windowTitle)
		if (strncmp(windowTitle,IW(window)->titlebuffer,TITLEBUFFERLEN) == 0) change = FALSE;
	}

	if (change)
	{
		if (window == GetPrivScreen(window->WScreen)->TitlebarBufferWin) GetPrivScreen(window->WScreen)->TitlebarBufferWin = 0;
		int_RefreshWindowFrame(window, REFRESHGAD_TOPBORDER, 0, DOUBLEBUFFER, IntuitionBase);
	}

	/* Change screen's title */
	change = TRUE;

	if ((screenTitle == window->ScreenTitle) || (screenTitle == (UBYTE *)~0L)) change = FALSE;

	if (change)
	{
		//LONG lock = LockIBase(0);

		window->ScreenTitle = screenTitle;
		
		if (window->Flags & WFLG_WINDOWACTIVE)
		{
			if (screenTitle)
				window->WScreen->Title = screenTitle;
			else
				window->WScreen->Title = window->WScreen->DefaultTitle;
		}
		//UnlockIBase(lock);
		RenderScreenBar(window->WScreen, FALSE, IntuitionBase);
	}

	UNLOCKWINDOWLAYERS(window);

}
