/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(struct servent *, getservbyname,

/*  SYNOPSIS */
        AROS_LHA(char *, name,  A0),
        AROS_LHA(char *, proto, A1),

/*  LOCATION */
        struct Library *, SocketBase, 39, BSDSocket)

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

    aros_print_not_implemented ("getservbyname");
#warning TODO: Write BSDSocket/getservbyname

    return NULL;

    AROS_LIBFUNC_EXIT

} /* getservbyname */
