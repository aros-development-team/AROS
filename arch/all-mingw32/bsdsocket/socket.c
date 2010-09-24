/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <sys/errno.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"

/*****************************************************************************

    NAME */

        AROS_LH3(int, socket,

/*  SYNOPSIS */
        AROS_LHA(int, domain,   D0),
        AROS_LHA(int, type,     D1),
        AROS_LHA(int, protocol, D2),

/*  LOCATION */
        struct TaskBase *, taskBase, 12, BSDSocket)

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

    aros_print_not_implemented ("socket");
    SetError(ENOSYS, taskBase);

    return -1;

    AROS_LIBFUNC_EXIT

} /* socket */
