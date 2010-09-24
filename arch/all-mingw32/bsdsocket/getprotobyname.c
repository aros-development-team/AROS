/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <sys/errno.h>
#include <netdb.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"
#include "netdb_util.h"

/*****************************************************************************

    NAME */

        AROS_LH1(struct protoent *, getprotobyname,

/*  SYNOPSIS */
        AROS_LHA(char *, name, A0),

/*  LOCATION */
        struct TaskBase *, taskBase, 41, BSDSocket)

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
    struct PROTOENT *wsentry;
    struct protoent *arosentry = NULL;
    int err = 0;

    Forbid();

    wsentry = WSgetprotobyname(name);
    D(bug("[getprotobyname] Name: %s, WinSock protoent: 0x%p\n", name, wsentry));

    if (wsentry)
    {
	arosentry = CopyProtoEntry(wsentry, taskBase->pool);
	D(bug("[getprorobyname] AROS protoent: 0x%p\n", arosentry));
	if (!arosentry)
	    err = ENOMEM;
    }
    else
    {
	err = WSAGetLastError() - WSABASEERR;
	D(bug("[getprotobyname] WinSock error %u\n", err));
    }

    Permit();

    if (arosentry)
    {
	if (taskBase->pe)
	    FreeProtoEntry(taskBase->pe, taskBase->pool);
	taskBase->pe = arosentry;
    }
    else
	SetError(err, taskBase);

    return arosentry;

    AROS_LIBFUNC_EXIT

} /* getprotobyname */
