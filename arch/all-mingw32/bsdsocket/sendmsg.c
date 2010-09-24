/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <sys/socket.h>

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, sendmsg,

/*  SYNOPSIS */
        AROS_LHA(int,                   s,     D0),
        AROS_LHA(const struct msghdr *, msg,   A0),
        AROS_LHA(int,                   flags, D1),

/*  LOCATION */
        struct Library *, SocketBase, 45, BSDSocket)

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

    aros_print_not_implemented ("sendmsg");
#warning TODO: Write BSDSocket/sendmsg

    return 0;

    AROS_LIBFUNC_EXIT

} /* sendmsg */
