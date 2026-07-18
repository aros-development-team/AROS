/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: dos64.library packet transport helpers.
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "dos64_intern.h"

/*
 * Send a packet to a filesystem handler and wait for the result.
 * Values travel in the standard DosPacket argument slots, which are
 * SIPTR-sized (i.e. 64-bit on 64-bit systems). Returns dp_Res1; if
 * res2 is non-NULL it receives dp_Res2 (the IoErr() value).
 */
SIPTR dos64_SendPkt(struct Dos64Base *DOS64Base, struct MsgPort *port, LONG action,
                    SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5,
                    SIPTR *res2)
{
    SIPTR res;

    res = DoPkt(port, action, arg1, arg2, arg3, arg4, arg5);
    if (res2)
        *res2 = IoErr();

    return res;
}

#if (__WORDSIZE != 64)
/*
 * On 32-bit systems the standard packet arguments cannot carry 64-bit
 * values, so the AmigaOS 4 style ACTION_*64 packets use the
 * struct DosPacket64 overlay (see <dos/dos64.h>): the object (fh_Arg1)
 * goes into the standard dp_Arg1 slot, dp_Res0 is set to DP64_INIT and
 * the 64-bit argument travels in the overlay's dp_Arg2.
 *
 * The overlay is larger than a standard DosPacket, so a private message
 * and packet pair is built here instead of using DOS_STDPKT.
 */
QUAD dos64_SendPkt64OS4(struct Dos64Base *DOS64Base, struct MsgPort *port, LONG action,
                        SIPTR object, QUAD arg64, LONG arg32, SIPTR *res2)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct
    {
        struct Message     msg;
        struct DosPacket64 pkt64;
    } *sp;
    struct DosPacket *dp;
    QUAD res;

    if (me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
    {
        SetIoErr(ERROR_ACTION_NOT_KNOWN);
        if (res2)
            *res2 = ERROR_ACTION_NOT_KNOWN;
        return -1;
    }

    sp = AllocVec(sizeof(*sp), MEMF_PUBLIC | MEMF_CLEAR);
    if (sp == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        if (res2)
            *res2 = ERROR_NO_FREE_STORE;
        return -1;
    }

    dp = (struct DosPacket *)&sp->pkt64;
    sp->msg.mn_Node.ln_Name = (char *)dp;
    sp->msg.mn_Length = sizeof(sp->pkt64);
    dp->dp_Link = &sp->msg;
    dp->dp_Type = action;
    dp->dp_Arg1 = object;               /* standard slot, as handlers expect  */
    sp->pkt64.dp_Res0 = DP64_INIT;      /* marks the packet as 64-bit capable */
    sp->pkt64.dp_Arg2 = arg64;
    sp->pkt64.dp_Arg3 = arg32;

    SendPkt(dp, port, &me->pr_MsgPort);
    if (WaitPkt() != dp)
        Alert(AN_AsyncPkt);

    res = sp->pkt64.dp_Res1;
    if (res2)
        *res2 = sp->pkt64.dp_Res2;

    FreeVec(sp);
    return res;
}
#endif
