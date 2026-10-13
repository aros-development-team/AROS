/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - the server process (re/spec/efs-protocol.md
          §5.1): one public entity "Filesystem" for every client, the
          mount and export-list transactions, the server-to-client channel
          (§4.5: action 5680 volume information / liveness ping, 5681
          notification events), the 1-second timer with the idle counters,
          and the reload of EFS.prefs on change.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <dos/var.h>
#include <string.h>

#include "fs_intern.h"
#include <proto/nipc.h>
#include <proto/security.h>
#include <libraries/security.h>
#include <envoy/errors.h>

#define NIPCBase    (srv->NipcLib)
#define UtilityBase (srv->UtilLib)
#define secBase     (srv->SecLib)

/*
 * Identity (envoy-aros-architecture.md §5.2): when security.library is
 * configured and the server runs as root, every client action is executed
 * with the mount user's effective uid and gid, so the local filesystem
 * enforces its own rules for the remote user. An unauthenticated mount
 * acts as nobody. Without root the server keeps its own identity and only
 * its own Full File Security checks apply.
 */
void ServerImpersonate(struct FSServer *srv, struct Mount *m)
{
    if (!srv->IsRoot)
        return;
    if (m->Authenticated)
    {
        secseteuid(m->Uid);
        secsetegid(m->Gid);
    }
    else
    {
        secseteuid(secOWNER_NOBODY >> 16);
        secsetegid(secOWNER_NOBODY & 0xFFFF);
    }
}

void ServerUnimpersonate(struct FSServer *srv)
{
    if (!srv->IsRoot)
        return;
    secseteuid(srv->OwnOwner >> 16);
    secsetegid(srv->OwnOwner & 0xFFFF);
}

ULONG ServerNewHandle(struct FSServer *srv)
{
    ULONG h;

    do
        h = srv->NextHandle++;
    while (h == 0);
    return h;
}

/* ---- records ------------------------------------------------------------- */

void LockRecFree(struct FSServer *srv, struct LockRec *l)
{
    if (l->Eac)
    {
        if (l->EacMore)
            ExAllEnd(l->Lock, l->EacBuf, l->EacBufSize, l->EacType, l->Eac);
        FreeDosObject(DOS_EXALLCONTROL, l->Eac);
    }
    FreeVec(l->EacBuf);
    FreeVec(l->EacPattern);
    if (l->Lock)
        UnLock(l->Lock);
    Remove((struct Node *)l);
    FreeVec(l);
}

void FileRecFree(struct FSServer *srv, struct FileRec *f)
{
    if (f->FH)
        Close(f->FH);
    Remove((struct Node *)f);
    FreeVec(f);
}

void NotifyRecFree(struct FSServer *srv, struct NotifyRec *n)
{
    if (n->Started)
        EndNotify(&n->NR);
    Remove((struct Node *)n);
    FreeVec(n);
}

void MountDestroy(struct FSServer *srv, struct Mount *m)
{
    struct LockRec *l;
    struct FileRec *f;
    struct NotifyRec *n;
    struct Event *ev;

    FSLOG(srv, "mount 0x%lx (%s on %s) destroyed\n", (unsigned long)m->ID, m->DevName, m->Host);
    while ((l = (struct LockRec *)GetHead(&m->Locks)))
        LockRecFree(srv, l);
    while ((f = (struct FileRec *)GetHead(&m->Files)))
        FileRecFree(srv, f);
    while ((n = (struct NotifyRec *)GetHead(&m->Notifies)))
        NotifyRecFree(srv, n);
    ForeachNode(&srv->Events, ev)
        if (ev->Mount == m)
            ev->Mount = NULL;
    if (m->Client)
        LoseEntity(m->Client);
    Remove((struct Node *)m);
    FreeVec(m);
}

/* ---- volume information block (§1.5) ---------------------------------------- */

void ServerBuildVolumeInfo(struct FSServer *srv, struct Mount *m, UBYTE *dst)
{
    struct Export *e = m->Export;
    ULONG flags;

    memset(dst, 0, 128);
    if (!e || !e->RootLock)
        return;                                     /* volume ID 0: no medium */
    flags = e->IsFS ? 4 : 0;
    if (GetHead(&m->Locks) || GetHead(&m->Files))
        flags |= 1;
    EfsPut32(dst, e->VolumeID);
    EfsPut32(dst + 4, flags);
    EfsPut32(dst + 8, e->VolDate.ds_Days);
    EfsPut32(dst + 12, e->VolDate.ds_Minute);
    EfsPut32(dst + 16, e->VolDate.ds_Tick);
    EfsPutCStr(dst + 20, 108, e->VolName);
}

/* ---- server -> client transactions (§4.5, §2.9) ------------------------------ */

void ServerSendEvent(struct FSServer *srv, struct Mount *m, ULONG action, ULONG key, BOOL idle)
{
    struct Transaction *t;
    struct Event *ev;
    ULONG len = (action == EFS_ACT_VOLUME) ? 180 : 564;
    UBYTE *req;

    if (!m->Client)
    {
        ULONG err = 0;
        m->Client = FindEntity(m->Host, m->DevName, srv->Ent, &err);
        if (!m->Client)
        {
            FSLOG(srv, "mount 0x%lx: client entity %s@%s not found (%lu)\n", (unsigned long)m->ID, m->DevName, m->Host, (unsigned long)err);
            if (action == EFS_ACT_VOLUME)
            {
                if (idle)
                    MountDestroy(srv, m);
                else
                    m->Idle = 1800;                 /* try again on the next tick, then give up */
            }
            return;
        }
    }
    if (!(ev = AllocVec(sizeof(struct Event), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    if (!(t = AllocTransaction(TRN_AllocReqBuffer, len, TAG_DONE)))
    {
        FreeVec(ev);
        return;
    }
    req = t->trans_RequestData;
    memset(req, 0, len);
    EfsPut32(req, m->ID);
    EfsPut32(req + 8, action);
    if (action == EFS_ACT_VOLUME)
        ServerBuildVolumeInfo(srv, m, req + 52);
    else
        EfsPut32(req + 12, key);                    /* Res1 = the client's notification key */
    t->trans_Command = 3;
    t->trans_ReqDataActual = len;
    t->trans_ResponseData = req;                    /* in place */
    t->trans_RespDataLength = len;
    t->trans_Timeout = 6;
    ev->Trans = t;
    ev->Mount = m;
    ev->Action = action;
    AddTail((struct List *)&srv->Events, (struct Node *)ev);
    if (!BeginTransaction(m->Client, srv->Ent, t))
    {
        Remove((struct Node *)ev);
        FreeVec(ev);
        FreeTransaction(t);
        LoseEntity(m->Client);
        m->Client = NULL;
        if (action == EFS_ACT_VOLUME && idle)
            MountDestroy(srv, m);
        return;
    }
    if (action == EFS_ACT_VOLUME)
    {
        m->PingOut = TRUE;
        m->PingIsIdle = idle;
    }
}

static void HandleEventResponse(struct FSServer *srv, struct Transaction *t)
{
    struct Event *ev;

    ForeachNode(&srv->Events, ev)
    {
        if (ev->Trans != t)
            continue;
        if (ev->Mount && ev->Action == EFS_ACT_VOLUME)
        {
            struct Mount *m = ev->Mount;
            BOOL ok = t->trans_Error == 0 && t->trans_RespDataActual >= 4 && EfsGet32(t->trans_ResponseData) == m->ID;
            m->PingOut = FALSE;
            if (ok)
                m->Idle = 0;
            else
            {
                FSLOG(srv, "mount 0x%lx: 5680 failed (error %lu)\n", (unsigned long)m->ID, (unsigned long)t->trans_Error);
                if (m->Client)
                    LoseEntity(m->Client);
                m->Client = NULL;
                if (m->PingIsIdle)
                    MountDestroy(srv, m);
            }
        }
        Remove((struct Node *)ev);
        FreeVec(ev);
        FreeTransaction(t);
        return;
    }
    FreeTransaction(t);                             /* not ours any more */
}

/* an EFS client changed an object: notifications the local filesystem could not watch */
void ServerNotifyChanged(struct FSServer *srv, struct Mount *m, CONST_STRPTR fullname)
{
    struct Mount *mm;
    struct NotifyRec *n;

    ForeachNode(&srv->Mounts, mm)
    {
        ForeachNode(&mm->Notifies, n)
        {
            if (!n->Started && !Stricmp(n->Name, fullname))
                ServerSendEvent(srv, mm, EFS_ACT_NOTIFYEVENT, n->ClientKey, FALSE);
        }
    }
}

/* ---- the mount transaction (§1.4) ------------------------------------------- */

static ULONG NewMountID(struct FSServer *srv)
{
    struct DateStamp ds;
    ULONG id;
    struct Mount *m;
    BOOL used;

    DateStamp(&ds);
    id = ds.ds_Days * 86400UL + ds.ds_Minute * 60UL + ds.ds_Tick * 50UL;
    do
    {
        used = (id == 0);
        ForeachNode(&srv->Mounts, m)
            if (m->ID == id)
                used = TRUE;
        if (used)
            id++;
    }
    while (used);
    return id;
}

static void HandleMount(struct FSServer *srv, struct Transaction *t)
{
    UBYTE *req = t->trans_RequestData, *resp = t->trans_ResponseData;
    ULONG len = t->trans_ReqDataActual, caps, pos = 16;
    char export[64], devname[256], user[36], password[36];
    struct UserInfo ui;
    struct Export *e;
    struct Mount *m, *old, *next;
    BOOL authenticated = FALSE, credfail = FALSE;

    if (len < 20 || t->trans_RespDataLength < 132)
    {
        t->trans_Error = EFSERR_REFUSED;
        t->trans_RespDataActual = 0;
        return;
    }
    caps = EfsGet32(req);
    EfsGetCStr(req, len, pos, export, sizeof(export));     pos += strlen(export) + 1;
    EfsGetCStr(req, len, pos, devname, sizeof(devname));   pos += strlen(devname) + 1;
    EfsGetCStr(req, len, pos, user, sizeof(user));         pos += strlen(user) + 1;
    EfsGetCStr(req, len, pos, password, sizeof(password));
    FSLOG(srv, "mount request: export '%s', device '%s', user '%s', caps 0x%lx\n", export, devname, user, (unsigned long)caps);

    e = AuthSelectExport(srv, export, user, password, &ui, &authenticated, &credfail);
    if (!e)
    {
        struct Export *named = ConfigFindExport(srv, export);
        EfsPut32(resp, 0);
        t->trans_RespDataActual = 4;
        /* 0x8000 "credentials rejected" where that is the reason (§1.4 recommendation), 0x8001 otherwise */
        t->trans_Error = (credfail && named && !(named->Flags & EXPF_NOSEC)) ? EFSERR_UNKNOWNCMD : EFSERR_REFUSED;
        FSLOG(srv, "mount refused: %s\n", credfail ? "bad credentials" : "no such export or not on the access list");
        return;
    }
    if (!(m = AllocVec(sizeof(struct Mount), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        EfsPut32(resp, 0);
        t->trans_RespDataActual = 4;
        t->trans_Error = EFSERR_REFUSED;
        return;
    }
    NEWLIST((struct List *)&m->Locks);
    NEWLIST((struct List *)&m->Files);
    NEWLIST((struct List *)&m->Notifies);
    if (!GetHostName(t->trans_SourceEntity, m->Host, sizeof(m->Host)))
        strcpy(m->Host, "?");
    EfsPutCStr((UBYTE *)m->DevName, sizeof(m->DevName), devname);
    EfsPutCStr((UBYTE *)m->User, sizeof(m->User), user);
    m->Source = t->trans_SourceEntity;
    m->Export = e;
    m->Authenticated = authenticated;
    m->Uid = ui.ui_UserID;
    m->Gid = ui.ui_PrimaryGroupID;
    m->AccFlags = ui.ui_Flags;
    m->Flags = MNTF_HIDEBACKDROP | MNTF_PROTDISKINFO | MNTF_FULLSEC;
    if (e->Flags & EXPF_LEFTOUT)   m->Flags &= ~MNTF_HIDEBACKDROP;
    if (e->Flags & EXPF_SNAPSHOT)  m->Flags &= ~MNTF_PROTDISKINFO;
    if (!(e->Flags & EXPF_FULLSEC)) m->Flags &= ~MNTF_FULLSEC;
    if (e->Flags & EXPF_READONLY)  m->Flags |= MNTF_READONLY;
    if (caps & 1)                  m->Flags |= MNTF_ABSPOS;

    /* a new mount from the same host under the same device name replaces the old one */
    ForeachNodeSafe(&srv->Mounts, old, next)
        if (!Stricmp(old->Host, m->Host) && !Stricmp(old->DevName, m->DevName))
            MountDestroy(srv, old);

    m->ID = NewMountID(srv);
    AddTail((struct List *)&srv->Mounts, (struct Node *)m);
    FSLOG(srv, "mount 0x%lx: '%s' for %s@%s as uid %u gid %u flags 0x%lx%s\n", (unsigned long)m->ID, e->Name,
          m->DevName, m->Host, m->Uid, m->Gid, (unsigned long)m->Flags, authenticated ? "" : " (not authenticated)");

    /* the volume information goes to the client's entity before the mount is answered */
    ServerSendEvent(srv, m, EFS_ACT_VOLUME, 0, FALSE);

    memset(resp, 0, 132);
    EfsPut32(resp, m->ID);
    ServerBuildVolumeInfo(srv, m, resp + 4);
    t->trans_RespDataActual = 132;
    t->trans_Error = 0;
}

/* ---- the export list (§1.6) --------------------------------------------------- */

static void HandleList(struct FSServer *srv, struct Transaction *t)
{
    UBYTE *req = t->trans_RequestData, *resp = t->trans_ResponseData;
    char user[64], password[64];
    struct UserInfo ui;
    struct Export *e;
    BOOL authenticated;
    ULONG n = 0;

    EfsGetCStr(req, t->trans_ReqDataActual, 0, user, sizeof(user));
    EfsGetCStr(req, t->trans_ReqDataActual, 64, password, sizeof(password));
    authenticated = AuthVerify(srv, user, password, &ui);
    ForeachNode(&srv->Exports, e)
    {
        if (!e->RootLock || !AuthMayMount(srv, e, &ui, authenticated))
            continue;
        if ((n + 1) * 64 > t->trans_RespDataLength)
            break;
        EfsPutCStr(resp + n * 64, 64, (e->Name[0] && e->Name[0] != ':' && !(e->Flags & EXPF_REMOVABLE)) ? e->Name : e->Path);
        n++;
    }
    FSLOG(srv, "list exports for '%s': %lu entries%s\n", user, (unsigned long)n, authenticated ? "" : " (login failed)");
    t->trans_RespDataActual = n * 64;
    t->trans_Error = 0;
}

/* ---- command 3 ---------------------------------------------------------------- */

static void HandlePacket(struct FSServer *srv, struct Transaction *t)
{
    UBYTE *req = t->trans_RequestData, *resp = t->trans_ResponseData;
    struct Mount *m = NULL, *mm;
    ULONG id;

    if (t->trans_ReqDataActual < EFS_HDR || t->trans_RespDataLength < EFS_HDR)
    {
        t->trans_Error = EFSERR_REFUSED;
        t->trans_RespDataActual = 0;
        return;
    }
    if (resp != req)
        CopyMem(req, resp, EFS_HDR);
    t->trans_RespDataActual = EFS_HDR;
    id = EfsGet32(req);
    ForeachNode(&srv->Mounts, mm)
        if (mm->ID == id)
            m = mm;
    if (!m || m->Source != t->trans_SourceEntity)
    {
        t->trans_Error = EFSERR_REFUSED;
        return;
    }
    if (!m->Export || !m->Export->RootLock)
    {
        t->trans_Error = EFSERR_REFUSED;            /* the export vanished with a reload */
        MountDestroy(srv, m);
        return;
    }
    m->Idle = 0;
    t->trans_Error = 0;
    ServerImpersonate(srv, m);
    ActionsHandle(srv, m, t);
    ServerUnimpersonate(srv);
}

static void Dispatch(struct FSServer *srv, struct Transaction *t)
{
    switch (t->trans_Command)
    {
    case 1:  HandleMount(srv, t); break;
    case 3:  HandlePacket(srv, t); break;
    case 4:  HandleList(srv, t); break;
    default:
        t->trans_Error = EFSERR_UNKNOWNCMD;
        t->trans_RespDataActual = 0;
        break;
    }
}

/* ---- timer, notifications, main loop ------------------------------------------ */

static void StartTimer(struct FSServer *srv)
{
    srv->Timer->tr_node.io_Command = TR_ADDREQUEST;
    srv->Timer->tr_time.tv_secs = 1;
    srv->Timer->tr_time.tv_micro = 0;
    SendIO(&srv->Timer->tr_node);
    srv->TimerPending = TRUE;
}

static void Tick(struct FSServer *srv)
{
    struct Mount *m, *next;

    ForeachNodeSafe(&srv->Mounts, m, next)
    {
        m->Idle++;
        if (m->Idle >= 1800 && !m->PingOut)
            ServerSendEvent(srv, m, EFS_ACT_VOLUME, 0, TRUE);
    }
}

static void WatchPrefs(struct FSServer *srv)
{
    memset(&srv->PrefsNR, 0, sizeof(srv->PrefsNR));
    srv->PrefsNR.nr_Name = (STRPTR)FS_PREFS_ENV;
    srv->PrefsNR.nr_Flags = NRF_SEND_MESSAGE;
    srv->PrefsNR.nr_stuff.nr_Msg.nr_Port = srv->NotifyPort;
    srv->PrefsWatched = StartNotify(&srv->PrefsNR);
}

/* ---- embedded Services Manager responder --------------------------------------
 *
 * Clients reach us through services.library FindService(): command 100 to the
 * public "Services Manager" entity (name at +0/64, user at +64/32, password at
 * +96/32; the 64-byte response carries the entity name - see
 * re/spec/services-accounts.md Â§2).  When Envoy's real ServicesManager is not
 * running, the daemon owns that entity itself and answers for its one service;
 * when the real manager owns it, the thin filesystem.service stub answers
 * StartServiceA() instead (fs_service.c).
 */
static void MgrHandleRequest(struct FSServer *srv, struct Transaction *t)
{
    ULONG err = 0;
    UBYTE *req = t->trans_RequestData;
    BOOL respok = t->trans_ResponseData && t->trans_RespDataLength >= 64;

    if (respok)
        memset(t->trans_ResponseData, 0, 64);
    if (t->trans_Command != 100 || !req || t->trans_ReqDataActual < 128 || !respok)
        err = ENVOYERR_BADSTARTSERVICE;
    else
    {
        char name[64];
        ULONG n = 0;

        while (n < sizeof(name) - 1 && req[n])
        {
            name[n] = req[n];
            n++;
        }
        name[n] = '\0';

        if (!Stricmp(name, FS_SERVICE_NAME))
            EfsPutCStr(t->trans_ResponseData, 64, FS_ENTITY_NAME);
        else
        {
            FSLOG(srv, "manager request for unknown service '%s'\n", name);
            err = ENVOYERR_UNKNOWNSERVICE;
        }
    }
    t->trans_Error = err;
    t->trans_RespDataActual = respok ? 64 : 0;
    ReplyTransaction(t);
}

int ServerMain(ULONG stopmask, ULONG beginmask, ULONG endmask)
{
    struct FSServer *srv;
    struct Mount *m;
    BOOL ok = FALSE, reload = FALSE;
    ULONG notifysig = 0, timersig = 0, mgrsig = 0;
    char var[8];

    if ((srv = AllocVec(sizeof(struct FSServer), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        NEWLIST((struct List *)&srv->Exports);
        NEWLIST((struct List *)&srv->Mounts);
        NEWLIST((struct List *)&srv->Events);
        srv->NextHandle = 0x100;
        srv->NextVolID = 0x10001;
        srv->Verbose = GetVar("Envoy/EFSQuiet", var, sizeof(var), GVF_GLOBAL_ONLY) <= 0;
        srv->UtilLib = OpenLibrary("utility.library", 36);
        srv->NipcLib = OpenLibrary(NIPCNAME, 50);
        srv->AccLib = OpenLibrary("accounts.library", 0);
        if ((srv->SecLib = OpenLibrary("security.library", 0)))
        {
            srv->OwnOwner = secGetTaskOwner(FindTask(NULL));
            srv->IsRoot = secIsConfigured() && (srv->OwnOwner >> 16) == secROOT_UID;
            FSLOG(srv, "security.library %s, server owner 0x%lx: client actions run %s\n",
                  secIsConfigured() ? "configured" : "not configured", (unsigned long)srv->OwnOwner,
                  srv->IsRoot ? "under the remote user's identity" : "with the server's own identity");
        }
        if (!srv->AccLib)
            FSLOG(srv, "accounts.library not available: only \"No Security\" exports can be mounted\n");
        if (srv->UtilLib && srv->NipcLib)
            srv->Ent = CreateEntity(ENT_Name, (IPTR)FS_ENTITY_NAME, ENT_Public, TRUE, ENT_AllocSignal, (IPTR)&srv->EntSig, TAG_DONE);
        if (srv->Ent && (srv->NotifyPort = CreateMsgPort()) && (srv->TimerPort = CreateMsgPort()) &&
            (srv->Timer = (struct timerequest *)CreateIORequest(srv->TimerPort, sizeof(struct timerequest))) &&
            !OpenDevice("timer.device", UNIT_VBLANK, &srv->Timer->tr_node, 0))
        {
            srv->TimerOpen = TRUE;
            ok = TRUE;
        }
        if (ok)
        {
            /* Self-sufficient discovery: own "Services Manager" unless the
             * real manager already does (a second public entity of the same
             * name cannot be created). */
            srv->MgrEnt = CreateEntity(ENT_Name, (IPTR)FS_MANAGER_ENTITY,
                                       ENT_Public, TRUE,
                                       ENT_AllocSignal, (IPTR)&srv->MgrSig,
                                       TAG_DONE);
            if (srv->MgrEnt)
                FSLOG(srv, "answering \"" FS_MANAGER_ENTITY "\" requests ourselves\n");
            else
                FSLOG(srv, "\"" FS_MANAGER_ENTITY "\" already served (ServicesManager runs)\n");
        }
    }

    if (ok)
    {
        notifysig = 1UL << srv->NotifyPort->mp_SigBit;
        timersig = 1UL << srv->TimerPort->mp_SigBit;
        if (srv->MgrEnt)
            mgrsig = 1UL << srv->MgrSig;
        WatchPrefs(srv);
        ConfigLoad(srv);
        StartTimer(srv);
        FSLOG(srv, "up, entity '%s'\n", FS_ENTITY_NAME);

        for (;;)
        {
            struct Transaction *t;
            struct NotifyMessage *nm;
            ULONG got = Wait((1UL << srv->EntSig) | mgrsig | notifysig | timersig
                             | SIGBREAKF_CTRL_C | stopmask | beginmask | endmask);

            if (got & (SIGBREAKF_CTRL_C | stopmask))
                break;
            if (got & beginmask)
            {
                /* reconfigure fence: no nipc traffic until the end signal */
                srv->Paused = TRUE;
                FSLOG(srv, "stack reconfigure: paused\n");
            }
            if (got & endmask)
            {
                srv->Paused = FALSE;
                reload = TRUE;              /* re-check exports, ping mounts */
                FSLOG(srv, "stack reconfigure done: resuming\n");
            }
            if (!srv->Paused)
            {
                while ((t = GetTransaction(srv->Ent)))
                {
                    if (t->trans_Type == TYPE_SERVICING)
                    {
                        Dispatch(srv, t);
                        ReplyTransaction(t);
                    }
                    else
                        HandleEventResponse(srv, t);
                }
                if (srv->MgrEnt)
                {
                    while ((t = GetTransaction(srv->MgrEnt)))
                    {
                        if (t->trans_Type == TYPE_SERVICING)
                            MgrHandleRequest(srv, t);
                        else
                            FreeTransaction(t);
                    }
                }
            }
            while ((nm = (struct NotifyMessage *)GetMsg(srv->NotifyPort)))
            {
                if (nm->nm_NReq == &srv->PrefsNR)
                    reload = TRUE;
                else if (!srv->Paused && nm->nm_NReq && nm->nm_NReq->nr_UserData)
                {
                    struct NotifyRec *n = (struct NotifyRec *)nm->nm_NReq->nr_UserData;
                    ServerSendEvent(srv, n->Mount, EFS_ACT_NOTIFYEVENT, n->ClientKey, FALSE);
                }
                ReplyMsg(&nm->nm_ExecMessage);
            }
            if (got & timersig)
            {
                while (GetMsg(srv->TimerPort))
                    ;
                srv->TimerPending = FALSE;
                if (!srv->Paused)
                {
                    Tick(srv);
                    if (reload)
                    {
                        reload = FALSE;
                        FSLOG(srv, "EFS.prefs changed: reloading\n");
                        ConfigLoad(srv);
                        ForeachNode(&srv->Mounts, m)
                            m->Idle = 1800;
                    }
                }
                StartTimer(srv);
            }
        }
    }

    if (srv)
    {
        struct Event *ev;
        while ((m = (struct Mount *)GetHead(&srv->Mounts)))
            MountDestroy(srv, m);
        while ((ev = (struct Event *)RemHead((struct List *)&srv->Events)))
        {
            AbortTransaction(ev->Trans);
            WaitTransaction(ev->Trans);
            FreeTransaction(ev->Trans);
            FreeVec(ev);
        }
        ConfigFree(srv);
        if (srv->PrefsWatched)
            EndNotify(&srv->PrefsNR);
        if (srv->TimerPending)
        {
            AbortIO(&srv->Timer->tr_node);
            WaitIO(&srv->Timer->tr_node);
        }
        if (srv->TimerOpen)
            CloseDevice(&srv->Timer->tr_node);
        if (srv->Timer)
            DeleteIORequest(&srv->Timer->tr_node);
        if (srv->TimerPort)
            DeleteMsgPort(srv->TimerPort);
        if (srv->NotifyPort)
            DeleteMsgPort(srv->NotifyPort);
        if (srv->MgrEnt)
            DeleteEntity(srv->MgrEnt);
        if (srv->Ent)
            DeleteEntity(srv->Ent);
        if (srv->SecLib)
            CloseLibrary(srv->SecLib);
        if (srv->AccLib)
            CloseLibrary(srv->AccLib);
        if (srv->NipcLib)
            CloseLibrary(srv->NipcLib);
        if (srv->UtilLib)
            CloseLibrary(srv->UtilLib);
        FreeVec(srv);
    }
    return ok ? 0 : RETURN_FAIL;
}
