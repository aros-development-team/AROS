/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, socket,

/*  SYNOPSIS */
        AROS_LHA(int, domain,   D0),
        AROS_LHA(int, type,     D1),
        AROS_LHA(int, protocol, D2),

/*  LOCATION */
        struct Library *, SocketBase, 5, BSDSocket)

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

    aros_print_not_implemented ("socket");
#warning TODO: Write BSDSocket/socket

    return NULL;

    AROS_LIBFUNC_EXIT

} /* socket */
