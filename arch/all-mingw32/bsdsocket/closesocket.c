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

    int err;

    D(bug("[bsdsocket] CloseSocket(%u)\n", s));

    err = IntCloseSocket(s, taskBase);
    if (err)
    {
	SetError(err, taskBase);
	return -1;
    }
    else
    {
	struct Socket *sd = taskBase->dTable[s];

	Remove((struct Node *)sd);
	FreePooled(taskBase->pool, sd, sizeof(struct Socket));

	taskBase->dTable[s] = NULL;
	return 0;
    }

    AROS_LIBFUNC_EXIT
} /* CloseSocket */

int IntCloseSocket(int s, struct TaskBase *taskBase)
{
    struct bsdsocketBase *SocketBase = taskBase->glob;
    struct Socket *sd;
    int err;

    if (s >= taskBase->dTableSize)
	return ENOTSOCK;

    sd = taskBase->dTable[s];
    if (!sd)
	return ENOTSOCK;

    Forbid();
    err = WSclosesocket(sd->s);
    if (err)
	err = WSAGetLastError() - WSABASEERR;
    Permit();
    D(bug("[CloseSocket] Closed socket %u, error %u\n", s, err));

    return err;
}
