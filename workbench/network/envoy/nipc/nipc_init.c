/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library initialisation, open/close, supervisor start and stop
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <string.h>

#include "nipc_intern.h"

static int NIPC_Init(struct NIPCBase *NIPCBase)
{
    InitSemaphore(&NIPCBase->OpenSem);
    InitSemaphore(&NIPCBase->Sem);
    NEWLIST(&NIPCBase->Entities);
    NEWLIST(&NIPCBase->Events);
    NEWLIST(&NIPCBase->Conns);
    NEWLIST(&NIPCBase->Ifaces);
    NEWLIST(&NIPCBase->Inquiries);
    NEWLIST(&NIPCBase->InqReplies);
    NEWLIST(&NIPCBase->Finders);
    NEWLIST(&NIPCBase->Pings);
    NEWLIST(&NIPCBase->Realms);
    NIPCBase->NextPort = NIPC_FIRST_DYNPORT;
    NIPCBase->NextQueryID = 1;

    if (!(NIPCBase->nipc_DOSBase = OpenLibrary("dos.library", 36)))
        return FALSE;
    if (!(NIPCBase->nipc_UtilityBase = OpenLibrary("utility.library", 36)))
        return FALSE;
    return TRUE;
}

static int NIPC_Expunge(struct NIPCBase *NIPCBase)
{
    if (NIPCBase->nipc_UtilityBase)
        CloseLibrary(NIPCBase->nipc_UtilityBase);
    if (NIPCBase->nipc_DOSBase)
        CloseLibrary(NIPCBase->nipc_DOSBase);
    return TRUE;
}

/*
 * The first opener starts the supervisor and waits until it is ready; the
 * last closer stops it. As in Envoy, a failed start makes OpenLibrary fail.
 */
static int NIPC_Open(struct NIPCBase *NIPCBase)
{
    BOOL ok = TRUE;

    ObtainSemaphore(&NIPCBase->OpenSem);
    if (!NIPCBase->Super)
        ok = StartSupervisor(NIPCBase);
    ReleaseSemaphore(&NIPCBase->OpenSem);
    return ok;
}

static int NIPC_Close(struct NIPCBase *NIPCBase)
{
    ObtainSemaphore(&NIPCBase->OpenSem);
    if (NIPCBase->LibNode.lib_OpenCnt == 0)
        StopSupervisor(NIPCBase);
    ReleaseSemaphore(&NIPCBase->OpenSem);
    return TRUE;
}

ADD2INITLIB(NIPC_Init, 0);
ADD2EXPUNGELIB(NIPC_Expunge, 0);
ADD2OPENLIB(NIPC_Open, 0);
ADD2CLOSELIB(NIPC_Close, 0);

BOOL StartSupervisor(struct NIPCBase *NIPCBase)
{
    struct MsgPort *reply;
    struct Message msg;
    struct TagItem tags[] =
    {
        { NP_Entry,     (IPTR)SuperProcess  },
        { NP_Name,      (IPTR)"NIPC"        },
        { NP_Priority,  10                  },
        { NP_StackSize, 65536               },
        { NP_UserData,  (IPTR)NIPCBase      },
        { TAG_DONE,     0                   }
    };

    if (NIPCBase->Super)
        return TRUE;
    if (!(reply = CreateMsgPort()))
        return FALSE;
    memset(&msg, 0, sizeof(msg));
    msg.mn_ReplyPort = reply;
    msg.mn_Length = sizeof(msg);
    NIPCBase->SuperOK = FALSE;

    if ((NIPCBase->Super = CreateNewProc(tags)))
    {
        PutMsg(&NIPCBase->Super->pr_MsgPort, &msg);
        WaitPort(reply);
        GetMsg(reply);
    }
    DeleteMsgPort(reply);
    if (!NIPCBase->SuperOK)
        NIPCBase->Super = NULL;
    return NIPCBase->SuperOK;
}

void StopSupervisor(struct NIPCBase *NIPCBase)
{
    struct SuperReq req;

    if (!NIPCBase->Super)
        return;
    memset(&req, 0, sizeof(req));
    req.Type = SREQ_QUIT;
    SendSuperReq(NIPCBase, &req);
    /* the supervisor clears Super and signals us when it is gone */
    NIPCBase->Closer = FindTask(NULL);
    while (NIPCBase->Super)
        Wait(SIGF_SINGLE);
    NIPCBase->Closer = NULL;
}
