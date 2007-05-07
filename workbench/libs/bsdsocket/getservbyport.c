/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(struct servent *, getservbyport,

/*  SYNOPSIS */
        AROS_LHA(int,    port,  D0),
        AROS_LHA(char *, proto, A0),

/*  LOCATION */
        struct Library *, SocketBase, 40, BSDSocket)

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

    aros_print_not_implemented ("getservbyport");
#warning TODO: Write BSDSocket/getservbyport

    return NULL;

    AROS_LIBFUNC_EXIT

} /* getservbyport */
