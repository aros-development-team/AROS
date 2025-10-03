/*
 * server.c -- Netinfo Server Process
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include <proto/exec.h>

#include "base.h"
#include "assert.h"

static void NetInfoPoll(struct NetInfoDevice *nid);

/*
 * Task Startup routine
 * This is called from DOS, so it has got no devbase
 */
 #if !defined(__AROS__)
#undef SysBase
#define SysBase (*(APTR *)4)
#endif

SAVEDS ASM LONG NetInfoStartup(void)
{
    struct NetInfoDevice *dev;
    struct Message *ok_message = (struct Message *)
                                 ((struct Process*)FindTask(NULL))->pr_ExitData;

    if (ok_message) {
        dev = (struct NetInfoDevice *) ok_message->mn_Node.ln_Name;
        if (dev) {
            NetInfoTask(dev, ok_message);
        } else {
            Forbid();
            ReplyMsg(ok_message);
        }
    }

    return 0;
}

void TermIO(struct NetInfoReq *req)
{
    if ((req->io_Flags & IOF_QUICK) == 0)
        ReplyMsg((struct Message *)req);
}
 #if !defined(__AROS__)
#undef SysBase
#define SysBase (nid->nid_ExecBase)
#endif

/*
 * Initialize device port
 */
void NetInfoTask(struct NetInfoDevice *nid, struct Message *ok_message)
{
    BYTE ps = AllocSignal(-1);
    struct Message *death;

    /* Create main message port */
    if (ps != -1) {

        /* Mr. Death */
        if (death = CreateIORequest(nid->nid_Port, sizeof(*death))) {
            struct NetInfoMap *maps[NETINFO_UNITS];
            struct NetInfoMap *mapp;
            short map;

            for (map = NETINFO_PASSWD_UNIT; map < NETINFO_UNITS; map++) {
                mapp = maps[map] = InitNetInfoMap(nid, nid->nid_Port, map);
                if (!mapp)
                    break;
            }

            /* If everything went well */
            if (map == NETINFO_UNITS) {
                /* Init ports */
                nid->nid_Port->mp_Flags = PA_SIGNAL;
                nid->nid_Port->mp_SigBit = ps;
                nid->nid_Port->mp_SigTask = FindTask(NULL);
                InitList(&nid->nid_Port->mp_MsgList);

                nid->nid_NotifyPort->mp_Flags = PA_SIGNAL;
                nid->nid_NotifyPort->mp_SigBit = ps;
                nid->nid_NotifyPort->mp_SigTask = FindTask(NULL);
                InitList(&nid->nid_NotifyPort->mp_MsgList);

                death->mn_Node.ln_Type = NT_MESSAGE;
                nid->nid_Death = death;
                for (map = NETINFO_PASSWD_UNIT; map < NETINFO_UNITS; map++)
                    nid->nid_Maps[map] = maps[map];

                ReplyMsg(ok_message);

                NetInfoPoll(nid);
                return;
            } else {
                while (map > 0) {
                    DeInitNetInfoMap(nid, maps[map--]);
                }
            }
            DeleteIORequest(death);
        }
        FreeSignal(ps);
    }

    ok_message->mn_Node.ln_Name = NULL;
    ReplyMsg(ok_message);
}

static void NetInfoPoll(struct NetInfoDevice *nid)
{
    ULONG mask = 1L << nid->nid_Port->mp_SigBit;

    while (mask) {
        struct NetInfoReq *req;
        struct NotifyMessage *notify;

        Wait(mask);

        while (req = (struct NetInfoReq *)GetMsg(nid->nid_Port)) {
            if (nid->nid_Death == &req->io_Message)
                mask = 0;
            else
                PerformIO(nid, req);
        }

        while (notify = (struct NotifyMessage *)GetMsg(nid->nid_NotifyPort)) {
            struct NetInfoMap *nim = (void *)notify->nm_NReq->nr_UserData;

            ReplyMsg((struct Message *)notify);

            if (nim != NULL) {
                Method(notify, nim)(nid, nim);
            }
        }
    }

    {
        /*
         * Free all allocated resources when die
         */
        struct NetInfoMap *mapp;
        short map;

        ObtainSemaphore(nid->nid_Lock);

        for (map = NETINFO_PASSWD_UNIT; map < NETINFO_UNITS; map++) {
            if (mapp = nid->nid_Maps[map])
                DeInitNetInfoMap(nid, mapp);
            nid->nid_Maps[map] = NULL;
        }

        if (nid->nid_SigBit != -1)
            FreeSignal(nid->nid_SigBit), nid->nid_SigBit = (UBYTE)-1;

        if (nid->nid_Death)
            DeleteIORequest(nid->nid_Death), nid->nid_Death = NULL;

        nid->nid_Task = NULL;

        ReleaseSemaphore(nid->nid_Lock);
        return;
    }
}

/*
 * Create netinfo pointer
 */
struct Unit *CreateNewUnit(struct NetInfoDevice *nid, short unit)
{
    struct NetInfoMap *nim = nid->nid_Maps[unit];
    struct NetInfoPointer *nip = AllocVec(sizeof(*nip), MEMF_CLEAR);

    if (nip) {
        nim->nim_OpenCnt++;
        nip->nip_Name = nim->nim_Name;
        nip->nip_UnitNumber = unit;
        nip->nip_Map = nim;
        nip->nip_Ent = NULL;
        ObtainSemaphore(nim->nim_PointerLock);
        AddHead(nim->nim_Pointer, nip->nip_Node);
        ReleaseSemaphore(nim->nim_PointerLock);
    }

    return (struct Unit *)nip;
}

/*
 * Delete netinfo pointer
 */
void ExpungeUnit(struct NetInfoDevice *nid, struct Unit *u)
{
    struct NetInfoPointer *nip = (struct NetInfoPointer *)u;
    struct NetInfoMap *nim = CheckUnit(nid, u);

    if (nim == NULL) {
        /* We should do an Alert */
        InMsg("CloseDevice(): illegal unit pointer %lx", u);
    } else {
        ObtainSemaphore(nim->nim_PointerLock);
        nim->nim_OpenCnt--;
        Remove(nip->nip_Node);
        ReleaseSemaphore(nim->nim_PointerLock);
        FreeVec(nip);
    }
    return;
}

void PerformIO(struct NetInfoDevice *nid, struct NetInfoReq *req)
{
    struct NetInfoMap *nim = CheckUnit(nid, req->io_Unit);

    if (nim) {
        DoNIMethod(req->io_Command, req, nim);
    } else {
        req->io_Error = IOERR_BADADDRESS;
        TermIO(req);
    }
}

ULONG AbortReq(struct NetInfoDevice *nid, struct List *l, struct NetInfoReq *req)
{
    struct Node *n;

    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ) {
        if (n == (struct Node *)req) {
            Remove((struct Node *)req);
            req->io_Error = IOERR_ABORTED;
            TermIO(req);
            return 0;
        }
    }

    return IOERR_NOCMD;
}

void UnknownCommand(struct NetInfoDevice *nid, struct NetInfoReq * req, struct NetInfoMap *nim)
{
    req->io_Error = IOERR_NOCMD;
    TermIO(req);
}
