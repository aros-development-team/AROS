/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(ULONG, SocketBaseTagList,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct Library *, SocketBase, 49, BSDSocket)

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

    aros_print_not_implemented ("SocketBaseTagList");
#warning TODO: Write BSDSocket/SocketBaseTagList

    return NULL;

    AROS_LIBFUNC_EXIT

} /* SocketBaseTagList */
