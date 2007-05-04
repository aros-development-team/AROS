/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(char *, Inet_NtoA,

/*  SYNOPSIS */
        AROS_LHA(unsigned long, in, D0),

/*  LOCATION */
        struct Library *, SocketBase, 29, BSDSocket)

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

    aros_print_not_implemented ("Inet_NtoA");
#warning TODO: Write BSDSocket/Inet_NtoA

    return NULL;

    AROS_LIBFUNC_EXIT

} /* Inet_NtoA */
