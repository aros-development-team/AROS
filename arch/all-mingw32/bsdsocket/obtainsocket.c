/*
    Copyright � 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH4(LONG, ObtainSocket,

/*  SYNOPSIS */
        AROS_LHA(LONG, id,       D0),
        AROS_LHA(LONG, domain,   D1),
        AROS_LHA(LONG, type,     D2),
        AROS_LHA(LONG, protocol, D3),

/*  LOCATION */
        struct Library *, SocketBase, 24, BSDSocket)

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

    aros_print_not_implemented ("ObtainSocket");
#warning TODO: Write BSDSocket/ObtainSocket

    return 0;

    AROS_LIBFUNC_EXIT

} /* ObtainSocket */
