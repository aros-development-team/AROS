/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(void, SetErrnoPtr,

/*  SYNOPSIS */
        AROS_LHA(void *, ptr,  A0),
        AROS_LHA(int,    size, D0),

/*  LOCATION */
        struct TaskBase *, taskBase, 28, BSDSocket)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    taskBase->errnoPtr = ptr;
    taskBase->errnoSize = size;

    AROS_LIBFUNC_EXIT
} /* SetErrnoPtr */
