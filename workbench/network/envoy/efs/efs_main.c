/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy filesystem client - handler start-up and main loop.
          Mounted from an Envoy mount entry:
            Unit="host¦export¦user¦$hash¦"   (separator byte 0xA6)
          The handler owns a public entity named after the DOS device, finds
          the "Filesystem" service on the host and mounts the export
          (re/spec/efs-protocol.md §1.2).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <string.h>

#include "efs_intern.h"
#include <proto/nipc.h>
#include <proto/services.h>

static void Cleanup(struct Globals *glob)
{
    struct EfsLock *l;
    struct EfsFile *f;

    RemoveVolumes(glob);
    while ((l = (struct EfsLock *)RemHead((struct List *)&glob->Locks)))
    {
        if (l->Scan)
            FreeVec(l->Scan);
        FreeVec(l);
    }
    while ((f = (struct EfsFile *)RemHead((struct List *)&glob->Files)))
    {
        struct EfsRecord *r;
        while ((r = (struct EfsRecord *)RemHead((struct List *)&f->Records)))
            FreeVec(r);
        FreeVec(f);
    }
    FreeNotifies(glob);
    FreeChunks(glob);
    if (glob->Trans)
        FreeTransaction(glob->Trans);
    if (glob->Service)
        LoseService(glob->Service);
    if (glob->Entity)
        DeleteEntity(glob->Entity);
    if (glob->NotifyPort)
        DeleteMsgPort(glob->NotifyPort);
    if (glob->gl_ServicesBase)
        CloseLibrary(glob->gl_ServicesBase);
    if (glob->gl_NIPCBase)
        CloseLibrary(glob->gl_NIPCBase);
    if (glob->gl_UtilityBase)
        CloseLibrary(glob->gl_UtilityBase);
    if (glob->gl_DOSBase)
        CloseLibrary((struct Library *)glob->gl_DOSBase);
}

static LONG Startup(struct Globals *glob, struct DosPacket *dp)
{
    struct FileSysStartupMsg *fssm = BADDR(dp->dp_Arg2);

    glob->DevNode = BADDR(dp->dp_Arg3);
    if (!(glob->gl_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36)))
        return ERROR_INVALID_RESIDENT_LIBRARY;
    if (!(glob->gl_UtilityBase = OpenLibrary("utility.library", 36)))
        return ERROR_INVALID_RESIDENT_LIBRARY;
    if (!(glob->gl_NIPCBase = OpenLibrary(NIPCNAME, 50)))
        return ERROR_INVALID_RESIDENT_LIBRARY;
    if (!(glob->gl_ServicesBase = OpenLibrary("services.library", 50)))
        return ERROR_INVALID_RESIDENT_LIBRARY;

    if (!glob->DevNode || !fssm || (IPTR)fssm->fssm_Unit < 65536 || !ParseUnit(glob, (CONST_STRPTR)fssm->fssm_Unit))
        return ERROR_BAD_STREAM_NAME;
    GetBstrArg(glob, glob->DevNode->dn_Name, glob->DevName, sizeof(glob->DevName));
    if (!glob->DevName[0])
        return ERROR_BAD_STREAM_NAME;
    EFSLOG("device '%s': host '%s' export '%s' user '%s' flags %lx\n", glob->DevName, glob->Host, glob->Export, glob->User, (unsigned long)glob->Flags);

    NEWLIST((struct List *)&glob->Locks);
    NEWLIST((struct List *)&glob->Files);
    NEWLIST((struct List *)&glob->Notifies);
    NEWLIST(&glob->Volumes);
    if (!(glob->NotifyPort = CreateMsgPort()))
        return ERROR_NO_FREE_STORE;
    {
        /* A handler of an earlier, dismounted mount of the same device may still own the
           entity name: it leaves within a few seconds (see DeviceListed()), so wait. */
        int tries;
        for (tries = 0; tries < 12; tries++)
        {
            if ((glob->Entity = CreateEntity(ENT_Name, (IPTR)glob->DevName, ENT_Public, TRUE,
                                             ENT_AllocSignal, (IPTR)&glob->EntSig, TAG_DONE)))
                break;
            Delay(25);
        }
        if (!glob->Entity)
            return ERROR_OBJECT_IN_USE;
    }
    if (!(glob->Trans = AllocTransaction(TRN_AllocReqBuffer, EFS_BUFSIZE, TAG_DONE)))
        return ERROR_NO_FREE_STORE;
    glob->Buf = glob->Trans->trans_RequestData;

    if (!FindFilesystem(glob) || !DoMount(glob))
        return MountErrorToDos(glob->LastError);
    return 0;
}

/*
 * Assign DISMOUNT only takes the device node out of the DOS list and frees it; the
 * handler keeps running, and with it the public entity named after the device, which
 * would make the next mount of the same export fail. So the handler looks every few
 * seconds whether its node is still listed, and leaves once it is gone and nothing is
 * open on it any more.
 */
static BOOL DeviceListed(struct Globals *glob)
{
    struct DosList *dl;
    BOOL found = FALSE;

    dl = LockDosList(LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
        if ((struct DeviceNode *)dl == glob->DevNode)
        {
            found = TRUE;
            break;
        }
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);
    return found;
}

LONG handler(struct ExecBase *sysbase)
{
    struct Globals bootstrap, *glob = &bootstrap;
    struct Process *proc;
    struct MsgPort *port;
    struct Message *msg;
    struct DosPacket *dp;
    LONG error;

    memset(&bootstrap, 0, sizeof(bootstrap));
    bootstrap.gl_SysBase = sysbase;
    proc = (struct Process *)FindTask(NULL);
    port = &proc->pr_MsgPort;
    WaitPort(port);
    msg = GetMsg(port);
    dp = (struct DosPacket *)msg->mn_Node.ln_Name;

    if (!(glob = AllocVec(sizeof(struct Globals), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        glob = &bootstrap;
        dp->dp_Res1 = DOSFALSE;
        dp->dp_Res2 = ERROR_NO_FREE_STORE;
        EfsReplyPkt(glob, dp);
        return RETURN_FAIL;
    }
    glob->gl_SysBase = sysbase;
    glob->Proc = proc;
    glob->Port = port;

    if ((error = Startup(glob, dp)))
    {
        EFSLOG("start-up failed: %ld\n", (long)error);
        Cleanup(glob);
        dp->dp_Res1 = DOSFALSE;
        dp->dp_Res2 = error;
        EfsReplyPkt(glob, dp);
        FreeVec(glob);
        return RETURN_FAIL;
    }

    glob->DevNode->dn_Task = port;
    dp->dp_Res1 = DOSTRUE;
    dp->dp_Res2 = 0;
    EfsReplyPkt(glob, dp);

    {
        ULONG pktsig = 1UL << port->mp_SigBit;
        ULONG entsig = 1UL << glob->EntSig;
        ULONG notsig = 1UL << glob->NotifyPort->mp_SigBit;
        ULONG timsig = 0;
        struct MsgPort *tport = CreateMsgPort();
        struct timerequest *treq = tport ? (struct timerequest *)CreateIORequest(tport, sizeof(struct timerequest)) : NULL;
        BOOL topen = treq && !OpenDevice("timer.device", UNIT_VBLANK, &treq->tr_node, 0);

        if (topen)
        {
            timsig = 1UL << tport->mp_SigBit;
            treq->tr_node.io_Command = TR_ADDREQUEST;
            treq->tr_time.tv_secs = 3;
            treq->tr_time.tv_micro = 0;
            SendIO(&treq->tr_node);
        }

        while (!glob->Quit)
        {
            ULONG sigs = Wait(pktsig | entsig | notsig | timsig);
            if (sigs & timsig)
            {
                WaitIO(&treq->tr_node);
                if (!DeviceListed(glob))
                {
                    glob->Dismounted = TRUE;
                    if (IsListEmpty((struct List *)&glob->Locks) && IsListEmpty((struct List *)&glob->Files))
                    {
                        EFSLOG("device dismounted: leaving\n");
                        glob->Quit = TRUE;
                        timsig = 0;
                        continue;
                    }
                }
                treq->tr_time.tv_secs = 3;
                treq->tr_time.tv_micro = 0;
                SendIO(&treq->tr_node);
            }
            if (sigs & entsig)
                HandleEntity(glob);
            if (sigs & pktsig)
            {
                while ((msg = GetMsg(port)))
                    HandlePacket(glob, (struct DosPacket *)msg->mn_Node.ln_Name);
            }
            if (sigs & notsig)
                HandleNotifyReplies(glob);
        }

        if (topen)
        {
            if (timsig)
            {
                AbortIO(&treq->tr_node);
                WaitIO(&treq->tr_node);
            }
            CloseDevice(&treq->tr_node);
        }
        if (treq)
            DeleteIORequest(&treq->tr_node);
        if (tport)
            DeleteMsgPort(tport);
    }

    EFSLOG("shutting down\n");
    if (!glob->Dismounted)
        glob->DevNode->dn_Task = NULL;      /* after Assign DISMOUNT the node is freed */
    Cleanup(glob);
    FreeVec(glob);
    return RETURN_OK;
}
