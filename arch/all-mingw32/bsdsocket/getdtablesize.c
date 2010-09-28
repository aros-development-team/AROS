/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH0(int, getdtablesize,

/*  SYNOPSIS */

/*  LOCATION */
        struct TaskBase *, taskBase, 23, BSDSocket)

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

    return taskBase->dTableSize;

    AROS_LIBFUNC_EXIT

} /* getdtablesize */
