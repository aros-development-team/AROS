
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <proto/security.h>

#include <exec/lists.h>
#include <exec/nodes.h>

#include "security_intern.h"
#include "security_memory.h"

/*
 *      Init Monitor List
 */

void InitMonList(struct SecurityBase *secBase)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)
    
    ObtainSemaphore(&secBase->MonitorSem);
    NEWLIST((struct List *)&secBase->MonitorList);
    ReleaseSemaphore(&secBase->MonitorSem);
}

/*
 *      Call the Monitors
 */

void CallMonitors(struct SecurityBase *secBase, ULONG triggerbit, UWORD from, UWORD to, char *userid)
{
    struct secMonitor *mon;
    struct secMonMsg *msg;
    
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    ObtainSemaphoreShared(&secBase->MonitorSem);
    for (mon = (struct secMonitor *)secBase->MonitorList.mlh_Head; mon->Node.mln_Succ;
              mon = (struct secMonitor *)mon->Node.mln_Succ)
        if (mon->Triggers & 1<<triggerbit)
            switch (mon->Mode) {
                case secMon_SEND_SIGNAL:
                        Signal(mon->u.Signal.Task, 1<<mon->u.Signal.SignalNum);
                        break;
                
                case secMon_SEND_MESSAGE:
                        if ((msg = MAlloc(sizeof(struct secMonMsg)))) {
                                msg->ExecMsg.mn_ReplyPort = secBase->MonitorPort;
                                msg->ExecMsg.mn_Length = sizeof(struct secMonMsg);
                                msg->Monitor = mon;
                                msg->Trigger = 1<<triggerbit;
                                msg->From = from;
                                msg->To = to;
                                if (userid) {
                                        strncpy(msg->UserID, userid, secUSERIDSIZE-1);
                                        msg->UserID[secUSERIDSIZE-1] = '\0';
                                }
                                PutMsg(mon->u.Message.Port,(struct Message*) msg);
                        }
                        break;
            }
    ReleaseSemaphore(&secBase->MonitorSem);
}


/*
 *      Free the replied-to Monitor Messages
 */

void FreeRepliedMonMsg(struct SecurityBase *secBase)
{
    struct secMonMsg *msg;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    while ((msg = (struct secMonMsg *)GetMsg(secBase->MonitorPort)))
        Free(msg, sizeof(struct secMonMsg));
}
