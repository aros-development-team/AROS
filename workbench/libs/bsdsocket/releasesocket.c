/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, ReleaseSocket,

/*  SYNOPSIS */
        AROS_LHA(LONG, sd, D0),
        AROS_LHA(LONG, id, D1),

/*  LOCATION */
        struct Library *, SocketBase, 25, BSDSocket)

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
    AROS_LIBBASE_EXT_DECL(struct Library *,SocketBase)

    aros_print_not_implemented ("ReleaseSocket");
#warning TODO: Write BSDSocket/ReleaseSocket

    return NULL;

    AROS_LIBFUNC_EXIT

} /* ReleaseSocket */
