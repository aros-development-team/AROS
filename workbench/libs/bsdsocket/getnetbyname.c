/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(struct netent *, getnetbyname,

/*  SYNOPSIS */
        AROS_LHA(char *, name, A0),

/*  LOCATION */
        struct Library *, SocketBase, 37, BSDSocket)

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

    aros_print_not_implemented ("getnetbyname");
#warning TODO: Write BSDSocket/getnetbyname

    return NULL;

    AROS_LIBFUNC_EXIT

} /* getnetbyname */
