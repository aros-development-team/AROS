/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH6(int, recvfrom,

/*  SYNOPSIS */
        AROS_LHA(int,               s,       D0),
        AROS_LHA(void *,            buf,     A0),
        AROS_LHA(int,               len,     D1),
        AROS_LHA(int,               flags,   D2),
        AROS_LHA(struct sockaddr *, from,    A1),
        AROS_LHA(int *,             fromlen, A2),

/*  LOCATION */
        struct Library *, SocketBase, 12, BSDSocket)

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

    aros_print_not_implemented ("recvfrom");
#warning TODO: Write BSDSocket/recvfrom

    return NULL;

    AROS_LIBFUNC_EXIT

} /* recvfrom */
