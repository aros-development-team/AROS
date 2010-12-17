/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL support
    Lang: english
*/

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

AROS_UFH2(BOOL, Dos_BCPL_putpkt,
/*  SYNOPSIS */
	AROS_UFHA(BSTR, bdospacket, D1),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))

/*  LOCATION
        BCPL Vector offset 0xa8

    FUNCTION

    INPUTS

    RESULT

    SEE ALSO

*****************************************************************************/
{
    AROS_USERFUNC_INIT

    struct Process *me   = (struct Process *)FindTask(NULL);
    struct DosPacket *dp = BADDR(bdospacket);
    struct Message *mess = dp->dp_Link;
    struct MsgPort *port;

    port = dp->dp_Port;

    dp->dp_Port = &me->pr_MsgPort;
    mess->mn_ReplyPort = dp->dp_Port;
    mess->mn_Node.ln_Name = (char *)dp;
    mess->mn_Length = sizeof(*mess);

bug("BCPL putPkt: Send to port %p, reply on port %p, DosPacket %p\n",
	port, mess->mn_ReplyPort, dp);

    PutMsg(port, mess);

    return TRUE;

    AROS_USERFUNC_EXIT
}

