/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - RA_DisposeWindowObject() implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <classes/window.h>

#include "reaction_intern.h"

/*****************************************************************************

    NAME */
#include <proto/reaction.h>

        AROS_LH1(BOOL, RA_DisposeWindowObject,

/*  SYNOPSIS */
        AROS_LHA(Object *, windowobj, A0),

/*  LOCATION */
        struct ReactionBase *, ReactionBase, 8, Reaction)

/*  FUNCTION
        Closes and disposes a window.class object and all its children.

    INPUTS
        windowobj - Pointer to a window.class object.

    RESULT
        TRUE if the object was successfully disposed.

    SEE ALSO
        RA_OpenWindow(), RA_CloseWindow()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (windowobj)
    {
        DisposeObject(windowobj);
        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
}
