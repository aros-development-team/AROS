/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - RA_CloseWindow() implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <classes/window.h>

#include "reaction_windowmethods.h"
#include "reaction_intern.h"

/*****************************************************************************

    NAME */
#include <proto/reaction.h>

        AROS_LH1(void, RA_CloseWindow,

/*  SYNOPSIS */
        AROS_LHA(Object *, windowobj, A0),

/*  LOCATION */
        struct ReactionBase *, ReactionBase, 6, Reaction)

/*  FUNCTION
        Closes a window previously opened with RA_OpenWindow().
        The window.class object is not disposed; it can be reopened later.

    INPUTS
        windowobj - Pointer to a window.class object.

    RESULT
        None.

    SEE ALSO
        RA_OpenWindow(), RA_DisposeWindowObject()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (windowobj)
    {
        DoMethod(windowobj, WM_CLOSE, NULL);
    }

    AROS_LIBFUNC_EXIT
}
