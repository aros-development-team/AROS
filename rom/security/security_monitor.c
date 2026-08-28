/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library monitors: notify interested tasks about logins,
          password changes and owner changes. Derived from MultiUser
          Monitor.c (c) Geert Uytterhoeven.
*/

#include <proto/exec.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_monitor.h"
#include "security_memory.h"

void InitMonList(struct SecurityBase *secBase)
{
    NEWLIST((struct List *)&secBase->MonitorList);
}

/*
 * Call the Monitors interested in triggerbit
 */
void CallMonitors(struct SecurityBase *secBase, ULONG triggerbit, UWORD from, UWORD to, CONST_STRPTR userid)
{
    struct secMonitor *mon;
    struct secMonMsg *msg;

    if (IsMinListEmpty(&secBase->MonitorList))
        return;

    ObtainSemaphoreShared(&secBase->MonitorSem);
    ForeachNode(&secBase->MonitorList, mon)
    {
        if (!(mon->Triggers & (1UL << triggerbit)))
            continue;
        switch (mon->Mode)
        {
        case secMon_SEND_SIGNAL:
            if (mon->u.Signal.Task)
                Signal(mon->u.Signal.Task, 1UL << mon->u.Signal.SignalNum);
            break;

        case secMon_SEND_MESSAGE:
            if (secBase->MonitorPort && mon->u.Message.Port && (msg = MAlloc(sizeof(struct secMonMsg))))
            {
                msg->ExecMsg.mn_ReplyPort = secBase->MonitorPort;
                msg->ExecMsg.mn_Length = sizeof(struct secMonMsg);
                msg->Monitor = mon;
                msg->Trigger = 1UL << triggerbit;
                msg->From = from;
                msg->To = to;
                if (userid)
                {
                    strncpy(msg->UserID, userid, secUSERIDSIZE - 1);
                    msg->UserID[secUSERIDSIZE - 1] = '\0';
                }
                PutMsg(mon->u.Message.Port, (struct Message *)msg);
            }
            break;
        }
    }
    ReleaseSemaphore(&secBase->MonitorSem);
}

/* Free the replied-to Monitor Messages (server context) */
void FreeRepliedMonMsg(struct SecurityBase *secBase)
{
    struct secMonMsg *msg;

    while ((msg = (struct secMonMsg *)GetMsg(secBase->MonitorPort)))
        Free(msg, sizeof(struct secMonMsg));
}
