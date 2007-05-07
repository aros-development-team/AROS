/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(int, Dup2Socket,

/*  SYNOPSIS */
        AROS_LHA(int, fd1, D0),
        AROS_LHA(int, fd2, D1),

/*  LOCATION */
        struct Library *, SocketBase, 44, BSDSocket)

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

    aros_print_not_implemented ("Dup2Socket");
#warning TODO: Write BSDSocket/Dup2Socket

    return NULL;

    AROS_LIBFUNC_EXIT

} /* Dup2Socket */
