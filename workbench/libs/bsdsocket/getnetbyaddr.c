/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(struct netent *, getnetbyaddr,

/*  SYNOPSIS */
        AROS_LHA(long, net,  D0),
        AROS_LHA(int,  type, D1),

/*  LOCATION */
        struct Library *, SocketBase, 38, BSDSocket)

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

    aros_print_not_implemented ("getnetbyaddr");
#warning TODO: Write BSDSocket/getnetbyaddr

    return NULL;

    AROS_LIBFUNC_EXIT

} /* getnetbyaddr */
