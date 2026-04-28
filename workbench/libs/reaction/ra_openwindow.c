/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - RA_OpenWindow() implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <classes/window.h>
#include <utility/tagitem.h>

#include "reaction_windowmethods.h"
#include "reaction_intern.h"

/*****************************************************************************

    NAME */
#include <proto/reaction.h>

        AROS_LH1(struct Window *, RA_OpenWindow,

/*  SYNOPSIS */
        AROS_LHA(Object *, windowobj, A0),

/*  LOCATION */
        struct ReactionBase *, ReactionBase, 5, Reaction)

/*  FUNCTION
        Opens a window previously created with WindowObject. This function
        sends the OM_OPEN method to the window.class object, which creates
        and opens the Intuition window with all child gadgets laid out.

    INPUTS
        windowobj - Pointer to a window.class object.

    RESULT
        Pointer to the opened Intuition Window structure, or NULL on failure.

    NOTES
        The window object must have been created with NewObject() using
        WINDOW_CLASSNAME ("window.class").

    SEE ALSO
        RA_CloseWindow(), RA_HandleInput()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Window *win = NULL;

    if (windowobj)
    {
        /* Send OM_OPEN to open the window. The window.class handles
           creating the Intuition window and laying out all children. */
        if (DoMethod(windowobj, WM_OPEN, NULL))
        {
            GetAttr(WINDOW_Window, windowobj, (IPTR *)&win);
        }
    }

    return win;

    AROS_LIBFUNC_EXIT
}
