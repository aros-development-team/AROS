/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFS client - mount entry, finding the service, the mount
          transaction, probing and reconnecting, volume records and the
          replay of locks and files (re/spec/efs-protocol.md §1, §4).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "efs_intern.h"
#include <proto/nipc.h>
#include <proto/services.h>

/* "host¦export¦user¦password¦[flags¦]", separator 0xA6, fields up to 79 characters */
BOOL ParseUnit(struct Globals *glob, CONST_STRPTR unit)
{
    char *fields[4] = { glob->Host, glob->Export, glob->User, glob->Password };
    const UBYTE *p = (const UBYTE *)unit;
    int f = 0;

    if (!unit)
        return FALSE;
    while (f < 5 && *p)
    {
        ULONG n = 0;
        while (*p && *p != 0xA6)
        {
            if (f < 4)
            {
                if (n < 79)
                    fields[f][n++] = *p;
            }
            else
            {
                switch (ToUpper(*p))
                {
                case 'B': glob->Flags |= EFSF_HIDEBACKDROP; break;
                case 'D': glob->Flags |= EFSF_PROTECTDISKINFO; break;
                case 'R': glob->Flags |= EFSF_NOREQUESTER; break;
                }
            }
            p++;
        }
        if (f < 4)
            fields[f][n] = '\0';
        if (*p)
            p++;
        f++;
    }
    return glob->Host[0] && glob->Export[0];
}

static UWORD Timeout(struct Globals *glob, ULONG mult)
{
    return (UWORD)((6 + glob->ExtraTimeout) * mult);
}

/* send the general-purpose buffer in place and wait */
static ULONG Transact(struct Globals *glob, UBYTE cmd, ULONG reqlen, ULONG tmult)
{
    struct Transaction *t = glob->Trans;

    t->trans_Command = cmd;
    t->trans_RequestData = glob->Buf;
    t->trans_ReqDataLength = EFS_BUFSIZE;
    t->trans_ReqDataActual = reqlen;
    t->trans_ResponseData = glob->Buf;
    t->trans_RespDataLength = EFS_BUFSIZE;
    t->trans_RespDataActual = 0;
    t->trans_Timeout = Timeout(glob, tmult);
    return DoTrans(glob, t);
}

BOOL FindFilesystem(struct Globals *glob)
{
    ULONG err = 0;
    struct TagItem tags[] = { { FSVC_Error, (IPTR)&err }, { TAG_DONE, 0 } };

    glob->Service = FindServiceA(glob->Host, "Filesystem", glob->Entity, tags);
    if (!glob->Service)
    {
        glob->LastError = err ? err : ENVOYERR_UNKNOWNENTITY;
        EFSLOG("FindService(%s, Filesystem) failed: %lu\n", glob->Host, (unsigned long)glob->LastError);
        return FALSE;
    }
    return TRUE;
}

/* trans_Command 1 (§1.4) */
BOOL DoMount(struct Globals *glob)
{
    UBYTE *b = glob->Buf, blk[EFS_VOLBLOCK];
    ULONG off, err;

    memset(b, 0, EFS_BUFSIZE);
    PutL(b, 0, 1);                              /* capability: absolute positions in READ/WRITE */
    off = PutCName(b, 16, glob->Export);
    off = PutCName(b, off, glob->DevName);
    off = PutCName(b, off, glob->User);
    PutCName(b, off, glob->Password);

    err = Transact(glob, EFSCMD_MOUNT, EFS_BUFSIZE, 1);
    if (err)
    {
        glob->LastError = err;
        EFSLOG("mount failed, trans_Error %lu\n", (unsigned long)err);
        return FALSE;
    }
    if (glob->Trans->trans_RespDataActual < 4 + EFS_VOLBLOCK || GetL(b, 0) == 0)
    {
        glob->LastError = EFSERR_REFUSED;
        EFSLOG("mount refused\n");
        return FALSE;
    }
    glob->MountID = GetL(b, 0);
    glob->Gen++;
    glob->Connected = TRUE;
    EFSLOG("mounted, mount ID %08lx, generation %u\n", (unsigned long)glob->MountID, glob->Gen);
    memcpy(blk, b + 4, EFS_VOLBLOCK);
    ProcessVolumeBlock(glob, blk);
    return TRUE;
}

/* action 31: any trans_Error 0 answer means "still connected" */
static void NoteTimeout(struct Globals *glob, ULONG err)
{
    /* §4.1: every ENVOYERR_TIMEOUT makes later transactions wait 6 s longer */
    if (err == ENVOYERR_TIMEOUT && glob->ExtraTimeout < 30)
        glob->ExtraTimeout += 6;
}

static BOOL Probe(struct Globals *glob)
{
    ULONG err;

    HdrSet(glob, glob->Buf, EFSACT_PROBE, 0, 0, 0, 0, 0, 0, 0);
    err = Transact(glob, EFSCMD_PACKET, EFS_BUFSIZE, 1);
    NoteTimeout(glob, err);
    EFSLOG("probe: trans_Error %lu, X now %lu\n", (unsigned long)err, (unsigned long)glob->ExtraTimeout);
    return err == 0;
}

/*
 * §4.3 step 3: probe the service entity we have; if that fails, give it up
 * and find the service and mount again. Up to four consecutive attempts;
 * the original then asks the user, this handler fails the packet (the
 * behaviour of mount flag R).
 */
BOOL Reconnect(struct Globals *glob)
{
    int attempt;

    if (glob->InReplay)
        return FALSE;
    for (attempt = 0; attempt < 4; attempt++)
    {
        if (glob->Service)
        {
            if (Probe(glob))
            {
                glob->Connected = TRUE;
                glob->FailedMounts = 0;
                return TRUE;
            }
            LoseService(glob->Service);
            glob->Service = NULL;
        }
        glob->Connected = FALSE;
        if (FindFilesystem(glob) && DoMount(glob))
        {
            glob->FailedMounts = 0;
            return TRUE;
        }
        if (glob->Service)
        {
            LoseService(glob->Service);
            glob->Service = NULL;
        }
        glob->FailedMounts++;
        if (glob->ExtraTimeout < 30)
            glob->ExtraTimeout += 6;
        EFSLOG("reconnect attempt %d failed\n", attempt + 1);
    }
    return FALSE;
}

/*
 * Send the EFS packet in the general-purpose buffer (header already set).
 * TRUE: dispatched by the server, *res1/*res2 are its DOS results.
 * FALSE: transport failure; *res2 holds the DOS error to report and
 * glob->Retry says whether the packet should be processed again after a
 * reconnect (handles may have changed).
 */
BOOL Forward(struct Globals *glob, ULONG action, ULONG reqlen, ULONG tmult, LONG *res1, LONG *res2)
{
    ULONG err;

    *res1 = 0;
    *res2 = ERROR_SEEK_ERROR;
    if (!glob->Connected)
    {
        if (Reconnect(glob))
            glob->Retry = TRUE;             /* redo the packet with the new handles */
        return FALSE;
    }
    PutL(glob->Buf, WH_MOUNTID, glob->MountID);
    err = Transact(glob, EFSCMD_PACKET, reqlen, tmult);
    if (err == 0)
    {
        *res1 = (LONG)GetL(glob->Buf, WH_RES1);
        *res2 = (LONG)GetL(glob->Buf, WH_RES2);
        if (++glob->OkCount >= 200)
        {
            glob->OkCount = 0;
            glob->ExtraTimeout = glob->ExtraTimeout >= 3 ? glob->ExtraTimeout - 3 : 0;
        }
        return TRUE;
    }
    EFSLOG("action %lu: trans_Error %lu\n", (unsigned long)action, (unsigned long)err);
    if (err == ENVOYERR_ABORTED)
        return FALSE;
    if (err == EFSERR_UNKNOWNCMD)
    {
        *res2 = ERROR_ACTION_NOT_KNOWN;
        return FALSE;
    }
    NoteTimeout(glob, err);
    if (err == ENVOYERR_NORESOURCES)
        *res2 = ERROR_NO_FREE_STORE;
    glob->Connected = FALSE;
    glob->Retry = TRUE;
    return FALSE;
}

LONG MountErrorToDos(ULONG err)
{
    switch (err)
    {
    case EFSERR_REFUSED:        return ERROR_OBJECT_NOT_FOUND;
    case ENVOYERR_NORESOURCES:  return ERROR_NO_FREE_STORE;
    default:                    return ERROR_DEVICE_NOT_MOUNTED;
    }
}

/* ---- volumes ------------------------------------------------------------ */

static struct EfsVolume *VolumeByID(struct Globals *glob, ULONG id)
{
    struct EfsVolume *v;
    ForeachNode(&glob->Volumes, v)
        if (v->ID == id)
            return v;
    return NULL;
}

static void AddDosVolume(struct Globals *glob, struct EfsVolume *v)
{
    char name[120];
    struct DosList *dl;
    int n = 0;

    CopyStr(name, sizeof(name), v->Name);
    dl = LockDosList(LDF_VOLUMES | LDF_READ);
    while (FindDosEntry(dl, name, LDF_VOLUMES) && n < 100)
    {
        /* the name is taken: "Name.1", "Name.2", ... */
        ULONG len = strlen(v->Name);
        if (len > 100) len = 100;
        memcpy(name, v->Name, len);
        name[len] = '.';
        n++;
        if (n >= 10) { name[len + 1] = '0' + n / 10; name[len + 2] = '0' + n % 10; name[len + 3] = '\0'; }
        else { name[len + 1] = '0' + n; name[len + 2] = '\0'; }
    }
    UnLockDosList(LDF_VOLUMES | LDF_READ);

    if ((dl = MakeDosEntry(name, DLT_VOLUME)))
    {
        dl->dol_Task = glob->Port;
        dl->dol_misc.dol_volume.dol_DiskType = 0x444F5380;      /* "DOS\x80": an EFS import */
        dl->dol_misc.dol_volume.dol_VolumeDate = v->Date;
        if (!AddDosEntry(dl))
        {
            FreeDosEntry(dl);
            dl = NULL;
        }
    }
    v->DosList = dl;
}

static void RemDosVolume(struct Globals *glob, struct EfsVolume *v)
{
    if (!v->DosList)
        return;
    LockDosList(LDF_VOLUMES | LDF_WRITE);
    RemDosEntry(v->DosList);
    UnLockDosList(LDF_VOLUMES | LDF_WRITE);
    FreeDosEntry(v->DosList);
    v->DosList = NULL;
}

void RemoveVolumes(struct Globals *glob)
{
    struct EfsVolume *v;
    while ((v = (struct EfsVolume *)RemHead(&glob->Volumes)))
    {
        RemDosVolume(glob, v);
        FreeVec(v);
    }
    glob->CurVol = NULL;
}

/* the 128-byte block of a mount response or an action 5680 (§1.5) */
void ProcessVolumeBlock(struct Globals *glob, const UBYTE *blk)
{
    ULONG id = GetL(blk, 0), flags = GetL(blk, 4);
    struct DateStamp ds;
    struct EfsVolume *v;
    char name[108];
    ULONG nlen;

    if (id == 0)
    {
        EFSLOG("volume block: no medium\n");
        glob->CurVol = NULL;
        return;
    }
    ds.ds_Days = GetL(blk, 8);
    ds.ds_Minute = GetL(blk, 12);
    ds.ds_Tick = GetL(blk, 16);
    nlen = strnlen((CONST_STRPTR)blk + 20, 107);
    memcpy(name, blk + 20, nlen);
    name[nlen] = '\0';

    ForeachNode(&glob->Volumes, v)
    {
        if (!Stricmp(v->Name, name) && v->Date.ds_Days == ds.ds_Days && v->Date.ds_Minute == ds.ds_Minute && v->Date.ds_Tick == ds.ds_Tick)
        {
            EFSLOG("volume '%s' is back as ID %08lx\n", name, (unsigned long)id);
            v->ID = id;
            v->Flags = flags;
            glob->CurVol = v;
            ReplayVolume(glob, v);
            return;
        }
    }
    if (!(v = AllocVec(sizeof(struct EfsVolume), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    v->ID = id;
    v->Flags = flags;
    v->Date = ds;
    CopyStr(v->Name, sizeof(v->Name), name);
    AddDosVolume(glob, v);
    AddTail(&glob->Volumes, &v->Node);
    glob->CurVol = v;
    EFSLOG("new volume '%s' ID %08lx flags %lx\n", name, (unsigned long)id, (unsigned long)flags);
}

/* ---- replay after a (re)mount (§4.4) ------------------------------------- */

BOOL RefreshLock(struct Globals *glob, struct EfsLock *l)
{
    ULONG end, err;
    LONG res1;

    if (l->Gen == glob->Gen)
        return TRUE;
    if (!glob->Connected)
        return FALSE;
    HdrSet(glob, glob->Buf, ACTION_LOCATE_OBJECT, 0xFFFFFFFFUL, 0, EFS_HDR, l->Mode, glob->Flags & 0xFFFF, 0, 0);
    end = PutBName(glob->Buf, EFS_HDR, l->Path);
    err = Transact(glob, EFSCMD_PACKET, end, 1);
    if (err || !(res1 = (LONG)GetL(glob->Buf, WH_RES1)))
        return FALSE;
    l->Handle = res1;
    l->VolID = GetL(glob->Buf, WH_RES2);
    l->Vol = VolumeByID(glob, l->VolID);
    l->Gen = glob->Gen;
    ResetScan(glob, l);
    return TRUE;
}

BOOL RefreshFile(struct Globals *glob, struct EfsFile *f)
{
    ULONG action = (f->OpenAction == ACTION_FINDINPUT) ? ACTION_FINDINPUT : ACTION_FINDUPDATE;
    struct EfsRecord *r;
    LONG res1;

    if (f->Gen == glob->Gen)
        return TRUE;
    if (!glob->Connected)
        return FALSE;
    HdrSet(glob, glob->Buf, action, 0xFFFFFFFFUL, 0, 0, EFS_HDR, glob->Flags & 0xFFFF, 0, 0);
    PutBName(glob->Buf, EFS_HDR, f->Path);
    if (Transact(glob, EFSCMD_PACKET, EFS_BUFSIZE, 1) || !(res1 = (LONG)GetL(glob->Buf, WH_RES1)))
        return FALSE;
    f->Handle = res1;
    f->VolID = GetL(glob->Buf, WH_RES2);
    f->Vol = VolumeByID(glob, f->VolID);
    f->Gen = glob->Gen;
    ForeachNode(&f->Records, r)
    {
        HdrSet(glob, glob->Buf, ACTION_LOCK_RECORD, 0xFFFFFFFFUL, f->Handle, r->Offset, r->Length, r->Mode, r->Timeout, 0);
        Transact(glob, EFSCMD_PACKET, EFS_BUFSIZE, 1);
    }
    HdrSet(glob, glob->Buf, ACTION_SEEK, 0xFFFFFFFFUL, f->Handle, f->Pos, (ULONG)-1, 0, 0, 0);
    Transact(glob, EFSCMD_PACKET, EFS_BUFSIZE, 1);
    return TRUE;
}

void ReplayVolume(struct Globals *glob, struct EfsVolume *v)
{
    struct EfsLock *l;
    struct EfsFile *f;
    struct EfsNotify *n;

    glob->InReplay = TRUE;
    ForeachNode(&glob->Locks, l)
        if (l->Vol == v && l->Gen != glob->Gen)
            RefreshLock(glob, l);
    ForeachNode(&glob->Files, f)
        if (f->Vol == v && f->Gen != glob->Gen)
            RefreshFile(glob, f);
    ForeachNode(&glob->Notifies, n)
    {
        ULONG end;
        HdrSet(glob, glob->Buf, ACTION_ADD_NOTIFY, 0xFFFFFFFFUL, n->Key, EFS_HDR, v->ID, n->NR->nr_Flags & ~NRF_NOTIFY_INITIAL, 0, 0);
        end = PutCName(glob->Buf, EFS_HDR, n->NR->nr_FullName);
        if (!Transact(glob, EFSCMD_PACKET, end, 1))
        {
            n->Handle = GetL(glob->Buf, WH_RES1);
            n->VolID = v->ID;
        }
    }
    glob->InReplay = FALSE;
}
