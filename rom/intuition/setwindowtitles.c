/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

#ifdef __MORPHOS__
#   include "renderwindowframe.h"
#endif

#include <string.h>

struct SetWindowTitlesActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    CONST_STRPTR    	     windowTitle;
    CONST_STRPTR    	     screenTitle;
};

static VOID int_setwindowtitles(struct SetWindowTitlesActionMsg *msg,
                               struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH3(void, SetWindowTitles,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window,      A0),
        AROS_LHA(CONST_STRPTR,    windowTitle, A1),
        AROS_LHA(CONST_STRPTR,    screenTitle, A2),

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SetWindowTitlesActionMsg msg;

    SANITY_CHECK(window)

    msg.window      = window;
    msg.windowTitle = windowTitle;
    msg.screenTitle = screenTitle;

    int_setwindowtitles(&msg, IntuitionBase);

    AROS_LIBFUNC_EXIT
} /* SetWindowTitles */

static VOID int_setwindowtitles(struct SetWindowTitlesActionMsg *msg,
                               struct IntuitionBase *IntuitionBase)
{
    struct Window   *window = msg->window;
    CONST_STRPTR     windowTitle = msg->windowTitle;
    CONST_STRPTR     screenTitle = msg->screenTitle;
    BOOL    	     change = TRUE;

    LOCKWINDOWLAYERS(window);

    /* Change window's title */
    if (windowTitle == (CONST_STRPTR)~0)
    {
         change = FALSE;
    }
    else
    {
        /* FIXME: Should we be strdup()ing this? */
        window->Title = (UBYTE *)windowTitle;
        if (windowTitle)
        if (strncmp(windowTitle,IW(window)->titlebuffer,TITLEBUFFERLEN) == 0) change = FALSE;
    }

    if (change)
    {
    #ifdef __MORPHOS__
        if (window == GetPrivScreen(window->WScreen)->TitlebarBufferWin) GetPrivScreen(window->WScreen)->TitlebarBufferWin = 0;
    #endif
        int_RefreshWindowFrame(window, REFRESHGAD_TOPBORDER, 0, DOUBLEBUFFER, IntuitionBase);
    }

    /* Change screen's title */
    if (screenTitle != (CONST_STRPTR)~0)
    {
        //LONG lock = LockIBase(0);

        /* FIXME: Should we be strdup()ing this? */
        window->ScreenTitle = (UBYTE *)screenTitle;
        
        if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
        {
            if (screenTitle)
        	/* FIXME: Should we be strdup()ing this? */
                window->WScreen->Title = (UBYTE *)screenTitle;
            else
                window->WScreen->Title = window->WScreen->DefaultTitle;

            RenderScreenBar(window->WScreen, FALSE, IntuitionBase);
        }
        //UnlockIBase(lock);
    }

    UNLOCKWINDOWLAYERS(window);

}
