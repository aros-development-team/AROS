/*
    Copyright © 2000-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH4(int, send,

/*  SYNOPSIS */
        AROS_LHA(int,          s,     D0),
        AROS_LHA(const void *, msg,   A0),
        AROS_LHA(int,          len,   D1),
        AROS_LHA(int,          flags, D2),

/*  LOCATION */
        struct Library *, SocketBase, 11, BSDSocket)

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

    aros_print_not_implemented ("send");
#warning TODO: Write BSDSocket/send

    return NULL;

    AROS_LIBFUNC_EXIT

} /* send */
