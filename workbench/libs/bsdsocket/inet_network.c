/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(unsigned long, inet_network,

/*  SYNOPSIS */
        AROS_LHA(const char *, cp, A0),

/*  LOCATION */
        struct Library *, SocketBase, 34, BSDSocket)

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

    aros_print_not_implemented ("inet_network");
#warning TODO: Write BSDSocket/inet_network

    return NULL;

    AROS_LIBFUNC_EXIT

} /* inet_network */
