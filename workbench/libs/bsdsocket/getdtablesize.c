/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
        struct Library *, SocketBase, 23, BSDSocket)

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

    aros_print_not_implemented ("getdtablesize");
#warning TODO: Write BSDSocket/getdtablesize

    return NULL;

    AROS_LIBFUNC_EXIT

} /* getdtablesize */
