/*
    Copyright © 2000-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define DEBUG 1

#include <proto/exec.h>

#include <sys/errno.h>
#include <sys/socket.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"
#include "socket_intern.h"

/* FIXME: AROS libc has incorrect values in errno.h */
#undef  EWOULDBLOCK
#define EWOULDBLOCK 35

/*****************************************************************************

    NAME */

        AROS_LH6(int, sendto,

/*  SYNOPSIS */
        AROS_LHA(int,                     s,     D0),
        AROS_LHA(const void *,            msg,   A0),
        AROS_LHA(int,                     len,   D1),
        AROS_LHA(int,                     flags, D2),
        AROS_LHA(const struct sockaddr *, to,    A1),
        AROS_LHA(int,                     tolen, D3),

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
    int res;
    int err = 0;
    struct WSsockaddr *sa;
    struct Socket *sd;

    D(bug("[sendto] Sending %u bytes to socket %u\n", len, s));
    sd = GetSocket(s, taskBase);
    if (!sd)
	return -1;

    D(bug("[sendto] Descriptor 0x%p, Windows socket %d\n", sd, sd->s));
    sa = MakeSockAddr(to, tolen, taskBase);
    if (!sa)
	return -1;

    Forbid();

    res = WSsendto(sd->s, msg, len, flags, sa, tolen);
    if (res == -1)
	err = WSAGetLastError() - WSABASEERR;

    Permit();
    FreePooled(taskBase->pool, sa, tolen);
    D(bug("[sendto] Result: %d, Error: %u\n", res, err));

    /* From Windows side all sockets are asynchronous, but from AROS side
       not all are. So if AROS socket is synchronous and Windows returned us
       EWOULDBLOCK, we need to wait for completion */
    if ((err == EWOULDBLOCK) && (!(sd->flags & SOF_NBIO)))
    {
	D(bug("[sendto] Waiting for the completion\n"));

    }

    if (res == -1)
	SetError(err, taskBase);

    return res;

    AROS_LIBFUNC_EXIT

} /* sendto */
