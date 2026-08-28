/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library packet helpers. Original code (c) 1998 Wez Furlong.
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_packetio.h"

/* The task that sent a packet, or NULL if it cannot be determined */
struct Task *GetPktTask(struct DosPacket *pkt)
{
    struct MsgPort *port;

    if (pkt && (port = pkt->dp_Port) && ((port->mp_Flags & PF_ACTION) == PA_SIGNAL))
        return port->mp_SigTask;
    return NULL;
}

/* Return the Owner of a DosPacket (allocated; free with secFreeExtOwner()) */
struct secExtOwner *GetPktOwner(struct SecurityBase *secBase, struct DosPacket *pkt)
{
    struct Task *task = GetPktTask(pkt);

    return task ? GetTaskExtOwner(secBase, task) : NULL;
}

/* Return the 'umask' of the task that sent a packet */
LONG GetPktDefProtection(struct SecurityBase *secBase, struct DosPacket *pkt)
{
    struct Task *task = GetPktTask(pkt);

    return task ? (LONG)GetTaskDefProtection(secBase, task) : secDEFPROTECTION;
}

/*
 * Send a packet to the real filesystem behind an enforced volume and wait
 * for the reply on the volume's private port.
 */
SIPTR secFSDoPkt(struct SecurityBase *secBase, struct secVolume *Vol, LONG act, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5, SIPTR *res2)
{
    struct StandardPacket StdPkt;

    memset(&StdPkt, 0, sizeof(StdPkt));
    StdPkt.sp_Msg.mn_ReplyPort = Vol->RepPort;
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
    do
    {
        WaitPort(Vol->RepPort);
    } while (GetMsg(Vol->RepPort) == NULL);

    if (res2)
        *res2 = StdPkt.sp_Pkt.dp_Res2;
    return StdPkt.sp_Pkt.dp_Res1;
}

/* Pass a packet straight through to the underlying FileSystem */
SIPTR DoPacket(struct SecurityBase *secBase, struct secVolume *Vol, struct DosPacket *pkt)
{
    SIPTR res2 = 0;

    pkt->dp_Res1 = secFSDoPkt(secBase, Vol, pkt->dp_Type, pkt->dp_Arg1, pkt->dp_Arg2, pkt->dp_Arg3, pkt->dp_Arg4, pkt->dp_Arg5, &res2);
    pkt->dp_Res2 = res2;
    return pkt->dp_Res1;
}
