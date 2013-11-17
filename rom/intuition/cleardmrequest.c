/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(BOOL, ClearDMRequest,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 8, Intuition)

/*  FUNCTION
        Detach the DMRequest from the window

    INPUTS
        window - The window from which the DMRequest is to be cleared

    RESULT
        TRUE if requester could successfully be detached.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetDMRequest(), Request()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return SetDMRequest(window, NULL);

    AROS_LIBFUNC_EXIT
} /* ClearDMRequest */
