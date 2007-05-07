/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, GetSocketEvents,

/*  SYNOPSIS */
        AROS_LHA(ULONG *, eventsp, A0),

/*  LOCATION */
        struct Library *, SocketBase, 50, BSDSocket)

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

    aros_print_not_implemented ("GetSocketEvents");
#warning TODO: Write BSDSocket/GetSocketEvents

    return NULL;

    AROS_LIBFUNC_EXIT

} /* GetSocketEvents */
