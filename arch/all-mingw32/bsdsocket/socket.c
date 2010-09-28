/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/exec.h>
#include <sys/errno.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"
#include "socket_intern.h"

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

    struct bsdsocketBase *SocketBase = taskBase->glob;
    struct Socket *sd;
    int err = 0;
    int s;

    D(bug("[bsdsocket] socket(%d, %d, %d)\n", domain, type, protocol));

    s = GetFreeFD(taskBase);
    if (s == -1)
	return -1;

    sd = AllocPooled(taskBase->pool, sizeof(struct Socket));
    if (!sd)
    {
	SetError(ENOMEM, taskBase);
	return -1;
    }

    sd->flags = 0;

    Forbid();

    sd->s = WSsocket(domain, type, protocol);
    if (sd->s == -1)
	err = WSAGetLastError();
    else
    {
	/* This implies setting the socket to non-blocking mode, so no ioctlsocket() is needed */
	err = WSAEventSelect(sd->s, SocketBase->ctl->SocketEvent, WS_FD_READ|WS_FD_WRITE|WS_FD_OOB|WS_FD_ACCEPT|WS_FD_CONNECT|WS_FD_CLOSE);
	if (err)
	{
	    err = WSAGetLastError();

	    WSclosesocket(sd->s);
	}
    }

    Permit();

    if (err)
    {
	FreePooled(taskBase->pool, sd, sizeof(struct Socket));
	SetError(err - WSABASEERR, taskBase);
	return -1;
    }

    AddTail((struct List *)&SocketBase->socks, (struct Node *)sd);
    taskBase->dTable[s] = sd;

    D(bug("[socket] Created socket %u (descriptor 0x%p, Windows socket %d)\n", s, sd, sd->s));
    return s;

    AROS_LIBFUNC_EXIT

} /* socket */
