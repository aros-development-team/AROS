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

AROS_UFH2(ULONG, Dos_BCPL_putpkt,
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

    D(struct Process *me   = (struct Process *)FindTask(NULL);)
    struct DosPacket *dp = BADDR(bdospacket);

    D(bug("BCPL putPkt: me->pr_MsgPort        = %p\n", &me->pr_MsgPort));
    D(bug("BCPL putPkt: me->pr_FileSystemTask = %p\n", me->pr_FileSystemTask));
    D(bug("BCPL putPkt: me->pr_ConsoleTask    = %p\n", me->pr_ConsoleTask));
#define DMP(x)  D(bug("BCPL putPkt: dp->dp_%s = %p\n", #x, dp->dp_##x))
    DMP(Link);
    DMP(Port);
    DMP(Type);
    DMP(Res1);
    DMP(Res2);
    DMP(Arg1);
    DMP(Arg2);
    DMP(Arg3);
    DMP(Arg4);
    DMP(Arg5);
    DMP(Arg6);
    DMP(Arg7);
#define MMP(x)  D(bug("BCPL putPkt: mn->mn_%s = %p\n", #x, dp->dp_Link->mn_##x))
    MMP(Node.ln_Succ);
    MMP(Node.ln_Pred);
    MMP(Node.ln_Type);
    MMP(Node.ln_Pri);
    MMP(Node.ln_Name);
    MMP(ReplyPort);
    MMP(Length);

    dp->dp_Link->mn_Node.ln_Name = (char *)dp;
    dp->dp_Link->mn_Length = sizeof(*dp->dp_Link);

    D(bug("BCPL putPkt: Send to port %p, reply on port %p, DosPacket %p\n",
	dp->dp_Port, &me->pr_MsgPort, dp));

    PutMsg(dp->dp_Port, dp->dp_Link);
    return DOSTRUE;

    AROS_USERFUNC_EXIT
}

