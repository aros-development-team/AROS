/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, accept,

/*  SYNOPSIS */
        AROS_LHA(int,               s,       D0),
        AROS_LHA(struct sockaddr *, addr,    A0),
        AROS_LHA(int *,             addrlen, A1),

/*  LOCATION */
        struct Library *, SocketBase, 8, BSDSocket)

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
    AROS_LIBBASE_EXT_DECL(struct Library *, SocketBase)

    aros_print_not_implemented ("accept");
#warning TODO: Write BSDSocket/accept

    return NULL;

    AROS_LIBFUNC_EXIT

} /* accept */
