/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(int, shutdown,

/*  SYNOPSIS */
        AROS_LHA(int, s,   D0),
        AROS_LHA(int, how, D1),

/*  LOCATION */
        struct Library *, SocketBase, 14, BSDSocket)

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

    aros_print_not_implemented ("shutdown");
#warning TODO: Write BSDSocket/shutdown

    return NULL;

    AROS_LIBFUNC_EXIT

} /* shutdown */
