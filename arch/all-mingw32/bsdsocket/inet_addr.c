/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(unsigned long, inet_addr,

/*  SYNOPSIS */
        AROS_LHA(const char *, cp, A0),

/*  LOCATION */
        struct TaskBase *, taskBase, 30, BSDSocket)

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

    struct bsdsocketBase *SocketBase = taskBase->glob;
    unsigned long res;

    Forbid();
    res = WSinet_addr(cp);
    Permit();

    return res;

    AROS_LIBFUNC_EXIT

} /* inet_addr */
