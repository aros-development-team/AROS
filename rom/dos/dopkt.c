/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH7(LONG, DoPkt,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, D1),
	AROS_LHA(LONG            , action, D2),
	AROS_LHA(LONG            , arg1, D3),
	AROS_LHA(LONG            , arg2, D4),
	AROS_LHA(LONG            , arg3, D5),
	AROS_LHA(LONG            , arg4, D6),
	AROS_LHA(LONG            , arg5, D7),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 40, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /*
     * First I create a regular dos packet and then let 
     * SendPkt rewrite it.
     */
    LONG res;
    struct Process * me = (struct Process *)FindTask(NULL);
    struct DosPacket * dp = (struct DosPacket *)AllocDosObject(DOS_STDPKT, NULL);
    struct MsgPort * replyport;
    BOOL i_am_process = TRUE;
    struct IOFileSys * iofs;
    
    if (NULL == dp)
      return FALSE;

    if (NT_PROCESS == me->pr_Task.tc_Node.ln_Type)
    {
      replyport = &me->pr_MsgPort;
    }
    else
    {
      replyport = CreateMsgPort();
      if (NULL == replyport)
        return FALSE;
      i_am_process = FALSE;
    }

    dp->dp_Type = action;
    dp->dp_Arg1 = arg1;
    dp->dp_Arg2 = arg2;
    dp->dp_Arg3 = arg3;
    dp->dp_Arg4 = arg4;
    dp->dp_Arg5 = arg5;
    
    SendPkt(dp, port, replyport);

    if (TRUE == i_am_process && NULL != me->pr_PktWait)
    {
#if 0
      dp = me->pr_PktWait();
#endif
    }
    else
    {
      WaitPort(replyport);
      iofs = (struct IOFileSys *)GetMsg(replyport);
//    dp = iofs->IOFS.io_oldPacket;
    }
    
    dp->dp_Res1 = DOSTRUE;
    dp->dp_Res2 = iofs->io_DosError;
    SetIoErr(iofs->io_DosError);

    res = dp->dp_Res1;

    if (FALSE == i_am_process)
      DeleteMsgPort(replyport);
    
    FreeDosObject(DOS_STDPKT, dp);

    return res;

    AROS_LIBFUNC_EXIT
} /* DoPkt */
