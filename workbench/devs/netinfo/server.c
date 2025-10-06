/*
 * server.c -- Netinfo Server Process
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "base.h"
#include "assert.h"

static void NetInfoPoll(struct NetInfoDevice *nid);

static const char dbuser_default[]  = "SYS:System/Network/AROSTCP/db/passwd";
static const char dbgroup_default[] = "SYS:System/Network/AROSTCP/db/group";

/*
 * Task Startup routine
 * This is called from DOS, so it has got no devbase
 */
SAVEDS ASM LONG NetInfoStartup(void)
{
    struct NetInfoDevice *dev;
    struct Message *ok_message = (struct Message *)
                                 ((struct Process*)FindTask(NULL))->pr_ExitData;

    D(bug("[NetInfo:Server] %s()\n", __func__));

    if (ok_message) {
        char buf[128];
        LONG len;

        dev = (struct NetInfoDevice *) ok_message->mn_Node.ln_Name;
        D(bug("[NetInfo:Server] %s: device @ 0x%p\n", __func__, dev));

        len = GetVar("SYS/userdb", buf, sizeof(buf), GVF_GLOBAL_ONLY);
        if (len > 0) {
            dev->nid_dbuser = StrDup(buf);
        } else
            dev->nid_dbuser = dbuser_default;
        len = GetVar("SYS/groupdb", buf, sizeof(buf), GVF_GLOBAL_ONLY);
        if (len > 0) {
            dev->nid_dbgroup = StrDup(buf);
        } else
            dev->nid_dbgroup = dbgroup_default;

        D(
            bug("[NetInfo:Server] %s: Database files -\n", __func__);
            bug("[NetInfo:Server] %s:     user  '%s'\n", __func__, dev->nid_dbuser);
            bug("[NetInfo:Server] %s:     group '%s'\n", __func__, dev->nid_dbgroup);
        )
        
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
    D(bug("[NetInfo:Server] %s(0x%p)\n", __func__, req));

    if ((req->io_Flags & IOF_QUICK) == 0)
        ReplyMsg((struct Message *)req);
}

/*
 * Initialize device port
 */
void NetInfoTask(struct NetInfoDevice *nid, struct Message *ok_message)
{
    BYTE ps = AllocSignal(-1);
    struct Message *death;

    D(bug("[NetInfo:Server] %s(0x%p, 0x%p)\n", __func__, nid, ok_message));

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
                NEWLIST(&nid->nid_Port->mp_MsgList);

                nid->nid_NotifyPort->mp_Flags = PA_SIGNAL;
                nid->nid_NotifyPort->mp_SigBit = ps;
                nid->nid_NotifyPort->mp_SigTask = FindTask(NULL);
                NEWLIST(&nid->nid_NotifyPort->mp_MsgList);

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

    D(bug("[NetInfo:Server] %s: exiting\n", __func__));

    if (nid->nid_dbuser && (nid->nid_dbuser != dbuser_default)) {
        FreeVec((APTR)nid->nid_dbuser);
        nid->nid_dbuser = NULL;
    }
    if (nid->nid_dbgroup && (nid->nid_dbgroup != dbgroup_default)) {
        FreeVec((APTR)nid->nid_dbgroup);
        nid->nid_dbgroup = NULL;
    }

    ok_message->mn_Node.ln_Name = NULL;
    ReplyMsg(ok_message);
}

static void NetInfoPoll(struct NetInfoDevice *nid)
{
    ULONG mask = 1L << nid->nid_Port->mp_SigBit;

    D(bug("[NetInfo:Server] %s(0x%p)\n", __func__, nid));

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

    D(bug("[NetInfo:Server] %s: device polled\n", __func__));

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
    }
}

/*
 * Create netinfo pointer
 */
struct Unit *CreateNewUnit(struct NetInfoDevice *nid, short unit)
{
    struct NetInfoMap *nim;
    struct NetInfoPointer *nip;

    D(bug("[NetInfo:Server] %s(0x%p, %d)\n", __func__, nid, unit));
    nim = nid->nid_Maps[unit];
    D(bug("[NetInfo:Server] %s: unit map @ 0x%p\n", __func__, nim));

    nip = AllocVec(sizeof(*nip), MEMF_CLEAR);
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

    D(bug("[NetInfo:Server] %s: returning 0x%p\n", __func__, nip));
    return (struct Unit *)nip;
}

/*
 * Delete netinfo pointer
 */
void ExpungeUnit(struct NetInfoDevice *nid, struct Unit *u)
{
    struct NetInfoPointer *nip = (struct NetInfoPointer *)u;
    struct NetInfoMap *nim;

    D(bug("[NetInfo:Server] %s(0x%p, 0x%p)\n", __func__, nid, u));

    nim = CheckUnit(nid, u);
    D(bug("[NetInfo:Server] %s: unit map @ 0x%p\n", __func__, nim));
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
    struct NetInfoMap *nim;

    D(bug("[NetInfo:Server] %s(0x%p, 0x%p)\n", __func__, nid, req));

    nim = CheckUnit(nid, req->io_Unit);
    D(bug("[NetInfo:Server] %s: unit map @ 0x%p\n", __func__, nim));
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

    D(bug("[NetInfo:Server] %s(0x%p, 0x%p, 0x%p)\n", __func__, nid, l , req));

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
    D(bug("[NetInfo:Server] %s(0x%p, 0x%p, 0x%p)\n", __func__, nid, req, nim));

    req->io_Error = IOERR_NOCMD;
    TermIO(req);
}
