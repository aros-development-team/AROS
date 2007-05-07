/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(unsigned long, Inet_LnaOf,

/*  SYNOPSIS */
        AROS_LHA(unsigned long, in, D0),

/*  LOCATION */
        struct Library *, SocketBase, 31, BSDSocket)

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

    aros_print_not_implemented ("Inet_LnaOf");
#warning TODO: Write BSDSocket/Inet_LnaOf

    return NULL;

    AROS_LIBFUNC_EXIT

} /* Inet_LnaOf */
