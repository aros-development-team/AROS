/*
    Copyright � 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH5(int, setsockopt,

/*  SYNOPSIS */
        AROS_LHA(int,    s,       D0),
        AROS_LHA(int,    level,   D1),
        AROS_LHA(int,    optname, D2),
        AROS_LHA(void *, optval,  A0),
        AROS_LHA(int,    optlen,  D3),

/*  LOCATION */
        struct Library *, SocketBase, 15, BSDSocket)

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

    aros_print_not_implemented ("setsockopt");
#warning TODO: Write BSDSocket/setsockopt

    return 0;

    AROS_LIBFUNC_EXIT

} /* setsockopt */
