/*
    Copyright (C) 2000-2010, The AROS Development Team. All rights reserved.

    Desc:
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, ReleaseCopyOfSocket,

/*  SYNOPSIS */
        AROS_LHA(LONG, sd, D0),
        AROS_LHA(LONG, id, D1),

/*  LOCATION */
        struct Library *, SocketBase, 26, BSDSocket)

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

    aros_print_not_implemented ("ReleaseCopyOfSocket");
#warning TODO: Write BSDSocket/ReleaseCopyOfSocket

    return 0;

    AROS_LIBFUNC_EXIT

} /* ReleaseCopyOfSocket */
