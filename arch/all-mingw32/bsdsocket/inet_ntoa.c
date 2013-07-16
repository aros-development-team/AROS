/*
    Copyright © 2000-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "bsdsocket_intern.h"
#include "netdb_util.h"

/*****************************************************************************

    NAME */

        AROS_LH1(char *, Inet_NtoA,

/*  SYNOPSIS */
        AROS_LHA(unsigned long, in, D0),

/*  LOCATION */
        struct TaskBase *, taskBase, 29, BSDSocket)

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
    char *res;
    struct in_addr a;

    D(bug("[bsdsocket] Inet_NToA(0x%08lX)\n", in));
    a.s_addr = in;

    Forbid();    

    res = WSinet_ntoa(a);
    if (res)
	res = CopyString(res, taskBase->pool);

    Permit();

    if (res)
    {
    FreeVecPooled(taskBase->inaddr, taskBase->pool);
	taskBase->inaddr = res;
    }

    Permit();

    D(bug("[inet_ntoa] Result: %s\n", res));
    return res;

    AROS_LIBFUNC_EXIT

} /* Inet_NtoA */
