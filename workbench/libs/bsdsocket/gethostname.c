/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(int, gethostname,

/*  SYNOPSIS */
        AROS_LHA(char *, name,    A0),
        AROS_LHA(int,    namelen, D0),

/*  LOCATION */
        struct Library *, SocketBase, 47, BSDSocket)

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

    aros_print_not_implemented ("gethostname");
#warning TODO: Write BSDSocket/gethostname

    return NULL;

    AROS_LIBFUNC_EXIT

} /* gethostname */
