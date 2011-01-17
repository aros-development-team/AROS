/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifdef DEBUG
#undef DEBUG
#endif
#define  DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

/* All KS versions accept most dos packet dos calls without dosbase in A6
 * So we don't call dos here either for compatibility purposes
 */

static struct DosPacket *allocdospacket(void)
{
    struct StandardPacket *sp = AllocVec(sizeof(struct StandardPacket), MEMF_CLEAR);

    if (sp == NULL)
	return NULL;

    sp->sp_Pkt.dp_Link = &(sp->sp_Msg);
    sp->sp_Msg.mn_Node.ln_Name = (char *) &(sp->sp_Pkt);

    return (APTR) &(sp->sp_Pkt);
}
static void freedospacket(struct DosPacket *dp)
{
	FreeVec((APTR)(((APTR)dp)-(APTR)(&((struct StandardPacket *)0)->sp_Pkt))); 
}

static SIPTR dopacket(SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5)
{
    /*
     * First I create a regular dos packet and then let 
     * SendPkt rewrite it.
     */

    SIPTR res;
    struct Process   *me = (struct Process *)FindTask(NULL);
    struct DosPacket *dp;
    struct MsgPort   *replyPort;
    BOOL i_am_process = TRUE;

    if (port == NULL) { /* NIL: ? */
    	D(bug("null port\n"));
    	return TRUE;
    }

    dp = allocdospacket();
    if (NULL == dp)
    	return FALSE;
    
    if (__is_process(me)) {
	replyPort = &me->pr_MsgPort;
    } else {
	/* Make sure that tasks can use DoPkt(). */
	replyPort = CreateMsgPort();

	if (NULL == replyPort) {
	    freedospacket(dp);
	    return FALSE;
	}

	i_am_process = FALSE;
    }
    
    D(bug("dp=%x act=%d port=%x reply=%x proc=%d %x %x %x %x %x\n",
    	dp, action, port, replyPort, i_am_process, arg1, arg2, arg3, arg4, arg5));
    dp->dp_Type = action;
    dp->dp_Arg1 = arg1;
    dp->dp_Arg2 = arg2;
    dp->dp_Arg3 = arg3;
    dp->dp_Arg4 = arg4;
    dp->dp_Arg5 = arg5;
    dp->dp_Res1 = 0;
    dp->dp_Res2 = 0;
    
    /* SendPkt */
    dp->dp_Port = replyPort;
    dp->dp_Link->mn_ReplyPort = replyPort;
    PutMsg(port, dp->dp_Link);

    while (GetMsg(replyPort) == NULL) {
        Wait(1 << replyPort->mp_SigBit);
    }

    if (res2)
    	*res2 = dp->dp_Res2;
    res = dp->dp_Res1;
    
    if (FALSE == i_am_process) {
	DeleteMsgPort(replyPort);
    } else {
    	me->pr_Result2 = dp->dp_Res2;
    }
    D(bug("res1=%x res2=%x\n", dp->dp_Res1, dp->dp_Res2));
    
    freedospacket(dp);
    return res;

}
SIPTR dopacket5(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5)
{
    return dopacket(res2, port, action, arg1, arg2, arg3, arg4, arg5);
}
SIPTR dopacket4(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4)
{
    return dopacket(res2, port, action, arg1, arg2, arg3, arg4, 0);
}
SIPTR dopacket3(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3)
{
    return dopacket(res2, port, action, arg1, arg2, arg3, 0, 0);
}
SIPTR dopacket2(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2)
{
    return dopacket(res2, port, action, arg1, arg2, 0, 0, 0);
}
SIPTR dopacket1(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1)
{
    return dopacket(res2, port, action, arg1, 0, 0, 0, 0);
}
SIPTR dopacket0(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action)
{
    return dopacket(res2, port, action, 0, 0, 0, 0, 0);
}

BOOL getpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR name, struct PacketHelperStruct *phs)
{
    if (isdosdevicec(name) < 0) { /* no ":" */
    	struct Process *me = (struct Process *)FindTask(NULL);
        BPTR cur;
    	BSTR bstrname = C2BSTR(name);
        struct FileLock *fl;
        cur = me->pr_CurrentDir;
        if (!cur)
            cur = DOSBase->dl_SYSLock;
        fl = BADDR(cur);
        phs->port = fl->fl_Task;
        phs->lock = cur;
        phs->dp = NULL;
        phs->name = bstrname;
        return TRUE;
    } else { /* ":" */
    	BSTR bstrname = C2BSTR(name);
        struct DevProc *dvp = NULL;
        if ((dvp = GetDeviceProc(name, dvp))) {
            phs->name = bstrname;
            phs->port = dvp->dvp_Port;
            phs->lock = dvp->dvp_Lock;
            phs->dp = dvp;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL getdevpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR devname, CONST_STRPTR name, struct PacketHelperStruct *phs)
{
    if ((phs->dp = GetDeviceProc(devname, NULL)) == NULL)
        return DOSFALSE;
    /* we're only interested in real devices */
    if (phs->dp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(phs->dp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }
    phs->port = phs->dp->dvp_Port;
    phs->lock = BNULL;
    phs->name = BNULL;
    if (!name)
    	return TRUE;
    phs->name = C2BSTR(name);
    if (!phs->name) {
        FreeDeviceProc(phs->dp);
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    return TRUE;
}

void freepacketinfo(struct DosLibrary *DOSBase, struct PacketHelperStruct *phs)
{
    if (phs->dp)
    	FreeDeviceProc(phs->dp);
    FreeVec(BADDR(phs->name));
}
