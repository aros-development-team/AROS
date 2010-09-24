/*
    Copyright � 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, IoctlSocket,

/*  SYNOPSIS */
        AROS_LHA(int,           s,       D0),
        AROS_LHA(unsigned long, request, D1),
        AROS_LHA(char *,        argp,    A0),

/*  LOCATION */
        struct Library *, SocketBase, 19, BSDSocket)

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

    aros_print_not_implemented ("IoctlSocket");
#warning TODO: Write BSDSocket/IoctlSocket

    return 0;

    AROS_LIBFUNC_EXIT

} /* IoctlSocket */
