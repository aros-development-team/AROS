
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include <dos/dos.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_packetio.h"

/*	Return the Owner of a DosPacket */

struct secExtOwner* GetPktOwner(struct SecurityBase *secBase, struct DosPacket* pkt)
{
    struct MsgPort* port;
    struct Task *task;
    struct secExtOwner *owner = NULL;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((port= pkt->dp_Port) && (port->mp_Flags == PA_SIGNAL) &&
                    (task = port->mp_SigTask))
        owner = secGetTaskExtOwner(task);
    return owner;
}

/* Return the owning task of a packet */

struct Task * GetPktTask(struct DosPacket *pkt)
{
    struct MsgPort* port;
    struct Task *task=NULL;
#if (0)
    struct secExtOwner *owner = NULL;
#endif

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((port= pkt->dp_Port) && (port->mp_Flags == PA_SIGNAL))
        task = port->mp_SigTask;
    return task;	
}

/*	Return the 'umask' of the task that sent a packet */

LONG GetPktDefProtection(struct SecurityBase *secBase, struct DosPacket *pkt)
{
    struct MsgPort* port;
    struct Task *task;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    LONG prot = DEFPROTECTION;
    if ((port= pkt->dp_Port) && (port->mp_Flags == PA_SIGNAL) &&
                    (task = port->mp_SigTask))
        prot = secGetDefProtection(task);
    return prot;
}

/* DoPkt */

SIPTR secFSDoPkt(struct secVolume *Vol, LONG act, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5)
{
    /* Send and Wait for a packet, using repport instead of pr_MsgPort for the
     * reply */

    struct StandardPacket StdPkt;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    StdPkt.sp_Msg.mn_ReplyPort = Vol->RepPort;
    StdPkt.sp_Msg.mn_Node.ln_Succ = NULL;
    StdPkt.sp_Msg.mn_Node.ln_Pred = NULL;
    StdPkt.sp_Msg.mn_Node.ln_Type = 0;
    StdPkt.sp_Msg.mn_Node.ln_Pri = 0;
    StdPkt.sp_Msg.mn_Node.ln_Name = (STRPTR)&StdPkt.sp_Pkt;
    StdPkt.sp_Msg.mn_Length = sizeof(struct StandardPacket);
    StdPkt.sp_Pkt.dp_Link = &StdPkt.sp_Msg;
    StdPkt.sp_Pkt.dp_Port = Vol->RepPort;
    StdPkt.sp_Pkt.dp_Type = act;
    StdPkt.sp_Pkt.dp_Arg1 = arg1;
    StdPkt.sp_Pkt.dp_Arg2 = arg2;
    StdPkt.sp_Pkt.dp_Arg3 = arg3;
    StdPkt.sp_Pkt.dp_Arg4 = arg4;
    StdPkt.sp_Pkt.dp_Arg5 = arg5;

    SendPkt(&StdPkt.sp_Pkt, Vol->OrigProc, Vol->RepPort);
    do {
            WaitPort(Vol->RepPort);
    } while(GetMsg(Vol->RepPort) == NULL);

    return StdPkt.sp_Pkt.dp_Res1;
}

/* DoPkt, based on the content of the packet; used to pass a packet straight
 * through to the underlying FileSystem */

SIPTR DoPacket(struct secVolume *Vol, struct DosPacket* pkt)
{
    SIPTR res1;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    res1 = secFSDoPkt(Vol, pkt->dp_Type, pkt->dp_Arg1, pkt->dp_Arg2, pkt->dp_Arg3, pkt->dp_Arg4, pkt->dp_Arg5);
    pkt->dp_Res1 = res1;
    pkt->dp_Res2 = IoErr();
    return res1;
}
