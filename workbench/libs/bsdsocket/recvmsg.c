/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, recvmsg,

/*  SYNOPSIS */
        AROS_LHA(int,             s,     D0),
        AROS_LHA(struct msghdr *, msg,   A0),
        AROS_LHA(int,             flags, D1),

/*  LOCATION */
        struct Library *, SocketBase, 46, BSDSocket)

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

    aros_print_not_implemented ("recvmsg");
#warning TODO: Write BSDSocket/recvmsg

    return NULL;

    AROS_LIBFUNC_EXIT

} /* recvmsg */
