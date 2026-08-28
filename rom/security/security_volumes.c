/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library volume discovery and configuration directory
          location. Runs in the server's context.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_memory.h"
#include "security_support.h"

extern const char PasswdFileName[];
extern const char GroupFileName[];

BOOL IsSecFSDosType(ULONG dostype)
{
    switch (dostype)
    {
    case ID_muFS_DISK:
    case ID_DOS_muFS_DISK:
    case ID_FFS_muFS_DISK:
    case ID_INTER_DOS_muFS_DISK:
    case ID_INTER_FFS_muFS_DISK:
    case ID_FASTDIR_DOS_muFS_DISK:
    case ID_FASTDIR_FFS_muFS_DISK:
    case ID_AFS_muFS_DISK:
    case ID_PFS2_muFS_DISK:
        return TRUE;
    }
    return FALSE;
}

/*
 * Find all volumes whose dostype marks them as multi-user aware
 */
/* Remember a volume/device name as natively enforcing (key file found) */
static void AddNativeVolume(struct SecurityBase *secBase, BSTR bname)
{
    struct secNativeVolume *nv;
    ULONG len = AROS_BSTR_strlen(bname);

    if (!len || len > 63)
        return;
    ForeachNode(&secBase->NativeVolumes, nv)
        if (strlen(nv->Name) == len && !Strnicmp(nv->Name, AROS_BSTR_ADDR(bname), len))
            return;
    if ((nv = MAllocV(sizeof(struct secNativeVolume) + len)))
    {
        CopyMem(AROS_BSTR_ADDR(bname), nv->Name, len);
        nv->Name[len] = '\0';
        AddTail((struct List *)&secBase->NativeVolumes, (struct Node *)&nv->Node);
        D(bug(DEBUG_NAME_STR " %s: '%s' enforces natively (key file found)\n", __func__, nv->Name);)
    }
}

static BOOL FindVolumes(struct SecurityBase *secBase)
{
    struct DosList *dl;
    struct FileSysStartupMsg *sm;
    struct DosEnvec *de;
    struct secVolume *vol;
    struct MsgPort *probe[64];
    BSTR probename[64];
    int nprobe = 0, i;
    BOOL res = FALSE;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    dl = LockDosList(LDF_DEVICES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
        if (dl->dol_Task &&
            (sm = BADDR(dl->dol_misc.dol_handler.dol_Startup)) && ((IPTR)sm > 1024) &&
            (de = BADDR(sm->fssm_Environ)) && (de->de_TableSize >= DE_DOSTYPE) &&
            !IsSecFSDosType(de->de_DosType))
        {
            /* A disk filesystem without a multi-user dostype: probe it for a
             * key file later (not while the DosList is locked) */
            if (nprobe < 64)
            {
                probe[nprobe] = dl->dol_Task;
                probename[nprobe++] = dl->dol_Name;
            }
        }
        else if (dl->dol_Task &&
            (sm = BADDR(dl->dol_misc.dol_handler.dol_Startup)) && ((IPTR)sm > 1024) &&
            (de = BADDR(sm->fssm_Environ)) && (de->de_TableSize >= DE_DOSTYPE) &&
            IsSecFSDosType(de->de_DosType))
        {
            if ((vol = (struct secVolume *)MAlloc(sizeof(struct secVolume))))
            {
                D(bug(DEBUG_NAME_STR " %s: New Volume @ %p for %p\n", __func__, vol, dl);)
                vol->DosList = dl;
                vol->Process = dl->dol_Task;
                vol->FS_Flags = 0;
                vol->Next = secBase->Volumes;
                secBase->Volumes = vol;
                res = TRUE;
            }
            else
                Die(secBase, NULL, AN_Unknown | AG_NoMemory);
        }
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);

    /* Volumes marked multi-user by a key file in their root (e.g. SFS) */
    for (i = 0; i < nprobe; i++)
    {
        if (ProbeKeyFile(secBase, probe[i]))
            AddNativeVolume(secBase, probename[i]);
    }
    return res;
}

/* Is this handler one of the volumes we track? */
BOOL IsSecFSVolume(struct SecurityBase *secBase, struct MsgPort *port)
{
    struct secVolume *vol;
    BOOL res = FALSE;

    ObtainSemaphoreShared(&secBase->VolumesSem);
    for (vol = secBase->Volumes; vol; vol = vol->Next)
        if (vol->Process == port || vol->OrigProc == port)
        {
            res = TRUE;
            break;
        }
    ReleaseSemaphore(&secBase->VolumesSem);
    return res;
}

/*
 * Use the default configuration directory (SYS:Security) and publish it
 * as SECURITY:. Returns FALSE if it does not exist (unconfigured system).
 */
static BOOL UseDefaultConfigDir(struct SecurityBase *secBase)
{
    BPTR lock;

    if ((lock = Lock(secConfig_DirName, ACCESS_READ)))
    {
        secBase->_cfgLock = lock;
        secBase->_pwdLock = DupLock(lock);
        AssignLock(secConfig_AssignName, DupLock(lock));
        D(bug(DEBUG_NAME_STR " %s: using %s\n", __func__, secConfig_DirName);)
        return TRUE;
    }
    D(bug(DEBUG_NAME_STR " %s: %s not found - unconfigured system\n", __func__, secConfig_DirName);)
    return FALSE;
}

static void StartNotifications(struct SecurityBase *secBase)
{
    BPTR olddir;

    if (secBase->_pwdLock)
    {
        olddir = CurrentDir(secBase->_pwdLock);
        memset(&secBase->PasswdNotifyReq, 0, sizeof(secBase->PasswdNotifyReq));
        secBase->PasswdNotifyReq.nr_Name = (char *)PasswdFileName;
        secBase->PasswdNotifyReq.nr_Flags = NRF_SEND_SIGNAL;
        secBase->PasswdNotifyReq.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        secBase->PasswdNotifyReq.nr_stuff.nr_Signal.nr_SignalNum = secBase->NotifySig;
        if (!StartNotify(&secBase->PasswdNotifyReq))
            secBase->PasswdNotifyReq.nr_Name = NULL;
        CurrentDir(olddir);
    }
    if (secBase->_cfgLock)
    {
        olddir = CurrentDir(secBase->_cfgLock);
        memset(&secBase->GroupNotifyReq, 0, sizeof(secBase->GroupNotifyReq));
        secBase->GroupNotifyReq.nr_Name = (char *)GroupFileName;
        secBase->GroupNotifyReq.nr_Flags = NRF_SEND_SIGNAL;
        secBase->GroupNotifyReq.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        secBase->GroupNotifyReq.nr_stuff.nr_Signal.nr_SignalNum = secBase->NotifySig;
        if (!StartNotify(&secBase->GroupNotifyReq))
            secBase->GroupNotifyReq.nr_Name = NULL;
        CurrentDir(olddir);
    }
}

/*
 * Initialise the Volume Information and locate the configuration.
 *
 * Order: key files on muFS volumes -> bootstrap rendezvous port ->
 * SYS:Security. Without any of them the system is "unconfigured": the
 * library is fully functional but everybody is treated as privileged.
 */
BOOL InitVolumes(struct SecurityBase *secBase)
{
    BOOL located = FALSE;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    if (!ClearBuffer(secBase))
        return FALSE;

    ObtainSemaphore(&secBase->VolumesSem);
    if (FindVolumes(secBase))
    {
        if (ReadKeyFiles(secBase))
            located = TRUE;
        else
        {
            secBase->SecurityViolation = TRUE;
            Die(secBase, GetLocStr(secBase, MSG_BADKEYFILE), 0);
        }
    }
    if (!located && !secBase->SecurityViolation)
        located = BootStrapRendezvous(secBase);
    if (!located && !secBase->SecurityViolation)
        located = UseDefaultConfigDir(secBase);
    ReleaseSemaphore(&secBase->VolumesSem);

    LoadConfig(secBase);

    /* Parse the database now so that Configured is valid */
    FreeDefs(secBase);
    GetUserDefs(secBase);

    if (located)
    {
        ReadFSTab(secBase);
        StartNotifications(secBase);
    }

    return TRUE;
}

/*
 * Free all Volume Information
 */
void FreeVolumes(struct SecurityBase *secBase)
{
    struct secVolume *vol, *next;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    FreeDefs(secBase);
    if (secBase->GroupNotifyReq.nr_Name)
    {
        EndNotify(&secBase->GroupNotifyReq);
        secBase->GroupNotifyReq.nr_Name = NULL;
    }
    if (secBase->PasswdNotifyReq.nr_Name)
    {
        EndNotify(&secBase->PasswdNotifyReq);
        secBase->PasswdNotifyReq.nr_Name = NULL;
    }
    if (secBase->_pwdLock)
    {
        UnLock(secBase->_pwdLock);
        secBase->_pwdLock = BNULL;
    }
    if (secBase->_cfgLock)
    {
        UnLock(secBase->_cfgLock);
        secBase->_cfgLock = BNULL;
    }
    ObtainSemaphore(&secBase->VolumesSem);
    {
        struct secNativeVolume *nv;
        while ((nv = (struct secNativeVolume *)RemHead((struct List *)&secBase->NativeVolumes)))
            FreeV(nv);
    }
    vol = secBase->Volumes;
    secBase->Volumes = NULL;
    while (vol)
    {
        next = vol->Next;
        if (vol->FS_Flags)
        {
            /* Preserve enforced volumes: their interceptor is still running */
            vol->Next = secBase->Volumes;
            secBase->Volumes = vol;
        }
        else
            Free(vol, sizeof(struct secVolume));
        vol = next;
    }
    ReleaseSemaphore(&secBase->VolumesSem);
    PurgeKeyBuffer(secBase);
    FreeBuffer(secBase);
}
