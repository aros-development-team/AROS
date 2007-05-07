/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH4(int, recv,

/*  SYNOPSIS */
        AROS_LHA(int,    s,     D0),
        AROS_LHA(void *, buf,   A0),
        AROS_LHA(int,    len,   D1),
        AROS_LHA(int,    flags, D2),

/*  LOCATION */
        struct Library *, SocketBase, 13, BSDSocket)

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

    aros_print_not_implemented ("recv");
#warning TODO: Write BSDSocket/recv

    return NULL;

    AROS_LIBFUNC_EXIT

} /* recv */
