/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH0(long, gethostid,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, SocketBase, 48, BSDSocket)

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

    aros_print_not_implemented ("gethostid");
#warning TODO: Write BSDSocket/gethostid

    return NULL;

    AROS_LIBFUNC_EXIT

} /* gethostid */
