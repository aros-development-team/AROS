/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <sys/errno.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"
#include "socket_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(int, CloseSocket,

/*  SYNOPSIS */
        AROS_LHA(int, s, D0),

/*  LOCATION */
        struct TaskBase *, taskBase, 20, BSDSocket)

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

    struct Socket *sd;

    D(bug("[bsdsocket] CloseSocket(%u)\n", s));

    sd = IntCloseSocket(s, taskBase);
    if (sd)
    {
	Remove((struct Node *)sd);
	FreePooled(taskBase->pool, sd, sizeof(struct Socket));

	taskBase->dTable[s] = NULL;
	return 0;
    }
    else
	return -1;

    AROS_LIBFUNC_EXIT
} /* CloseSocket */

struct Socket *IntCloseSocket(int s, struct TaskBase *taskBase)
{
    struct Socket *sd = GetSocket(s, taskBase);

    if (sd)
    {
        struct bsdsocketBase *SocketBase = taskBase->glob;
	int err;

	Forbid();
	err = WSclosesocket(sd->s);
	if (err)
	    err = WSAGetLastError() - WSABASEERR;
	Permit();
	D(bug("[CloseSocket] Closed socket %u, error %u\n", s, err));
	
	if (err)
	{
	    SetError(err, taskBase);
	    sd = NULL;
	}
    }

    return sd;
}
