/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(void, ClearPointer,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 10, Intuition)

/*  FUNCTION
        Reset the mouse pointer of this window to the default one. If the
        window is active during this call the pointer will immediately change
        its shape. Set custom mouse pointers with SetPointer().

    INPUTS
        window - The window of which the mousepointer will be cleared

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetPointer()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DEBUG_SETPOINTER(dprintf("ClearPointer: window 0x%lx\n",window));

    if( window )
    {
        window->Pointer = NULL;
        SetWindowPointerA(window, NULL);
    }

    AROS_LIBFUNC_EXIT
} /* ClearPointer */
