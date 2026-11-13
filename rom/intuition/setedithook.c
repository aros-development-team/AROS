/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(struct Hook *, SetEditHook,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 82, Intuition)

/*  FUNCTION
        Sets the global (default) string editing hook of Intuition
        string gadgets.

    INPUTS
        The string gadget editing hook to replace the old one.

    RESULT
        The old edit hook.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Hook *oldhook = GetPrivIBase(IntuitionBase)->GlobalEditHook;

    GetPrivIBase(IntuitionBase)->GlobalEditHook = hook;

    return (oldhook);

    AROS_LIBFUNC_EXIT
} /* SetEditHook */
