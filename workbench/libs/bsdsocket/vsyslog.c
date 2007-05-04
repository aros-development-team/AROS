/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(void, vsyslog,

/*  SYNOPSIS */
        AROS_LHA(int,          level,  D0),
        AROS_LHA(const char *, format, A0),
        AROS_LHA(LONG *,       args,   A1),

/*  LOCATION */
        struct Library *, SocketBase, 43, BSDSocket)

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
    AROS_LIBBASE_EXT_DECL(struct Library *,SocketBase)

    aros_print_not_implemented ("vsyslog");
#warning TODO: Write BSDSocket/vsyslog

    return;

    AROS_LIBFUNC_EXIT

} /* vsyslog */
