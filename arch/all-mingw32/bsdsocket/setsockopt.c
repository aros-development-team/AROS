/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"
#include "socket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH5(int, setsockopt,

/*  SYNOPSIS */
        AROS_LHA(int,    s,       D0),
        AROS_LHA(int,    level,   D1),
        AROS_LHA(int,    optname, D2),
        AROS_LHA(void *, optval,  A0),
        AROS_LHA(int,    optlen,  D3),

/*  LOCATION */
        struct TaskBase *, taskBase, 15, BSDSocket)

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

    struct Socket *sd = GetSocket(s, taskBase);
    int res = -1;
    int err;
    
    if (sd)
    {
	struct bsdsocketBase *SocketBase = taskBase->glob;

	Forbid();

	res = WSsetsockopt(sd->s, level, optname, optval, optlen);
	if (res)
	    err = WSAGetLastError() - WSABASEERR;

	Permit();

	if (res)
	    SetError(err, taskBase);
    }

    return res;

    AROS_LIBFUNC_EXIT

} /* setsockopt */
