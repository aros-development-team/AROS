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
    int s = GetFreeFD(taskBase);

    if (s == -1)
	return -1;

    sd = AllocPooled(taskBase->pool, sizeof(struct Socket));
    if (!sd)
    {
	SetError(ENOMEM, taskBase);
	return -1;
    }

    Forbid();

    sd->s = WSsocket(domain, type, protocol);
    if (sd->s == -1)
	err = WSAGetLastError();
    else
    {
	err = WSAEventSelect(sd->s, SocketBase->ctl->SocketEvent, FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE);
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

    D(bug("[socket] Created socket %u (descriptor 0x%p)\n", s, sd));
    return s;

    AROS_LIBFUNC_EXIT

} /* socket */
