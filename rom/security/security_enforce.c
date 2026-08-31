/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library filesystem enforcer: the access control policy
          (IsAllowed) and the packet interceptor used for filesystems that
          are not multi-user aware. Original code (c) 1998 Wez Furlong.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_memory.h"
#include "security_support.h"

#define FSTAB_TEMPLATE  "VOLUME/A,ROOTPROTECTION/K,ROOTUID/K/N,ROOTGID/K/N,READONLY=RO/K/S,NOSUID/K/S"

#define argVOLNAME      0
#define argROOTPROT     1
#define argROOTUID      2
#define argROOTGID      3
#define argREADONLY     4
#define argNOSUID       5

struct InterceptorArgs
{
    struct SecurityBase *secBase;
    STRPTR              volname;
    LONG                fs_flags;
    ULONG               owner;
    LONG                protection;
};

/*
 * Despatch table. Handlers implementing individual actions on top of the
 * proxy filesystem are still to be ported (see MultiUser2 FileSystem/);
 * everything not listed here is passed straight through.
 */
static const struct secFSE_PktHandler despatch[] =
{
    { 0, NULL }
};

/*
 * Access Control: decide whether 'task' may perform 'access' on an object
 * owned by 'object' with protection bits 'prot'.
 *
 * Note the Amiga protection encoding: the owner bits (FIBF_READ...) are
 * set when access is DENIED, the group/other bits are set when access is
 * GRANTED.
 */
LONG IsAllowed(struct SecurityBase *secBase, struct secVolume *Vol, struct secExtOwner *task,
               ULONG object, LONG prot, LONG access)
{
    /* A volume with a bad or inconsistent key file is quarantined: it is
     * treated as if it were not attached - nobody, root included, gets in. */
    if (Vol && Vol->Quarantined)
        return secAC_PERMISSION_DENIED | secAC_ROOT_DENIED;

    LONG retval = 0;
    ULONG who;
    LONG owner_deny = 0, grp_allow = 0, otr_allow = 0;

    /* An unconfigured system has no users: everything is allowed */
    if (!secBase->Configured)
        return secAC_PERMISSION_GRANTED;

    who = secGetRelationshipA(task, object, NULL);

    D(bug(DEBUG_NAME_STR " %s: object %08lx task %08lx access %ld prot %08lx rel %lx\n", __func__,
          (unsigned long)object, (unsigned long)secExtOwner2ULONG(task), (long)access, (unsigned long)prot, (unsigned long)who);)

    if (Vol && (Vol->FS_Flags & secFSE_READONLY))
    {
        /* Remove all write permissions */
        prot |= FIBF_WRITE;
        prot &= ~(FIBF_GRP_WRITE | FIBF_OTR_WRITE);
    }

    /* Root is always allowed, except on a read-only volume */
    if (who & secRelF_ROOT_UID)
    {
        if (Vol && (Vol->FS_Flags & secFSE_READONLY) && (access & (secAt_Write | secAt_Delete)))
            return secAC_PERMISSION_DENIED | secAC_READ_ONLY_FS | secAC_ROOT_DENIED;
        return secAC_PERMISSION_GRANTED;
    }

    /* Build the deny/allow masks for the requested access types */
    if (access & secAt_Read)
    {
        owner_deny |= FIBF_READ;
        grp_allow |= FIBF_GRP_READ;
        otr_allow |= FIBF_OTR_READ;
    }
    if (access & secAt_Write)
    {
        owner_deny |= FIBF_WRITE;
        grp_allow |= FIBF_GRP_WRITE;
        otr_allow |= FIBF_OTR_WRITE;
    }
    if (access & secAt_Execute)
    {
        owner_deny |= FIBF_EXECUTE;
        grp_allow |= FIBF_GRP_EXECUTE;
        otr_allow |= FIBF_OTR_EXECUTE;
    }
    if (access & secAt_Delete)
    {
        owner_deny |= FIBF_DELETE;
        grp_allow |= FIBF_GRP_DELETE;
        otr_allow |= FIBF_OTR_DELETE;
    }

    if (who & (secRelF_UID_MATCH | secRelF_NO_OWNER))
    {
        if ((prot & owner_deny) == 0)
            return secAC_PERMISSION_GRANTED;
        retval |= secAC_OWNER_DENIED;
    }
    if (who & secRelF_GID_MATCH)
    {
        if ((prot & grp_allow) == grp_allow)
            return secAC_PERMISSION_GRANTED;
        retval |= secAC_GROUP_DENIED;
    }
    if ((prot & otr_allow) == otr_allow)
        return secAC_PERMISSION_GRANTED;

    D(bug(DEBUG_NAME_STR " %s: ACCESS DENIED\n", __func__);)
    return secAC_PERMISSION_DENIED | retval;
}

/*
 * Bootstrap rendezvous: a program may tell us where the configuration is
 * by publishing a port before the server starts.
 */
BOOL BootStrapRendezvous(struct SecurityBase *secBase)
{
    struct secFSEnforceRendezvous *rndv;

    if ((rndv = (struct secFSEnforceRendezvous *)FindPort(secENFORCE_PORTNAME)))
    {
        secBase->_pwdLock = Lock(rndv->PasswdDir, ACCESS_READ);
        secBase->_cfgLock = Lock(rndv->ConfigDir, ACCESS_READ);

        if (secBase->_pwdLock && secBase->_cfgLock)
            return TRUE;

        if (secBase->_pwdLock) UnLock(secBase->_pwdLock);
        if (secBase->_cfgLock) UnLock(secBase->_cfgLock);
        secBase->_pwdLock = secBase->_cfgLock = BNULL;
    }
    return FALSE;
}

/*
 * Packet interceptor
 */
static int ConstructMountData(struct SecurityBase *secBase, struct secVolume *Vol, struct DeviceNode *dn2)
{
    char devname[256];
    struct DeviceNode *dn;
    BSTR bname = dn2->dn_Name;
    ULONG len = AROS_BSTR_strlen(bname);

    if (len >= sizeof(devname))
        len = sizeof(devname) - 1;
    CopyMem(AROS_BSTR_ADDR(bname), devname, len);
    devname[len] = '\0';

    if ((dn = (struct DeviceNode *)MakeDosEntry(devname, DLT_DEVICE)))
    {
        dn->dn_Task = Vol->Process;
        dn->dn_SegList = BNULL;
        dn->dn_GlobalVec = (BPTR)(SIPTR)-1;
        Vol->ProxyDosList = dn;
        AddDosEntry((struct DosList *)dn);
        return TRUE;
    }
    return FALSE;
}

static int ConstructVolumeData(struct SecurityBase *secBase, struct secVolume *Vol, struct DeviceList *dl2)
{
    char volname[256];
    struct DeviceList *dl;
    BSTR bname = dl2->dl_Name;
    ULONG len = AROS_BSTR_strlen(bname);

    if (len >= sizeof(volname))
        len = sizeof(volname) - 1;
    CopyMem(AROS_BSTR_ADDR(bname), volname, len);
    volname[len] = '\0';

    if ((dl = (struct DeviceList *)MakeDosEntry(volname, DLT_VOLUME)))
    {
        dl->dl_Task = Vol->Process;
        dl->dl_DiskType = dl2->dl_DiskType;
        dl->dl_VolumeDate = dl2->dl_VolumeDate;
        Vol->ProxyDosListVolume = dl;
        AddDosEntry((struct DosList *)dl);
        return TRUE;
    }
    return FALSE;
}

/* The Packet Intercepting/Relaying Loop */
static void InterceptorLoop(struct SecurityBase *secBase, struct secVolume *iVol)
{
    struct secExtOwner *owner;
    BOOL handled;
    LONG action, i;
    struct Message *mes;
    struct DosPacket *pkt;
    struct MsgPort *ivPort = iVol->Process;

    for (;;)
    {
        while ((mes = GetMsg(ivPort)) == NULL)
            WaitPort(ivPort);

        pkt = (struct DosPacket *)mes->mn_Node.ln_Name;
        action = pkt->dp_Type;
        owner = GetPktOwner(secBase, pkt);
        handled = FALSE;

        for (i = 0; despatch[i].action != 0; i++)
        {
            if ((despatch[i].action == action) && despatch[i].func)
            {
                handled = despatch[i].func(secBase, iVol, pkt, owner);
                break;
            }
        }
        if (!handled)
            DoPacket(secBase, iVol, pkt);

        ReplyPkt(pkt, pkt->dp_Res1, pkt->dp_Res2);

        if (owner)
            secFreeExtOwner(owner);
    }
}

/*
 * The interceptor process: takes over the DosList entries of a volume and
 * relays every packet to the real handler after checking it.
 */
static void InterceptorProcess(void)
{
    struct Process *ipProc = (struct Process *)FindTask(NULL);
    struct InterceptorArgs *ia = (struct InterceptorArgs *)ipProc->pr_Task.tc_UserData;
    struct SecurityBase *secBase = ia->secBase;
    struct FileInfoBlock *fib = NULL;
    struct MsgPort *RepPort = NULL;
    struct secVolume *secFSVolume, *vol;
    struct DevProc *DevProc = NULL;
    APTR winptrsave;
    int i;

    winptrsave = ipProc->pr_WindowPtr;
    ipProc->pr_WindowPtr = (APTR)-1;

    D(bug(DEBUG_NAME_STR " %s: FS Enforcer for '%s' starting\n", __func__, ia->volname);)

    ObtainSemaphore(&secBase->VolumesSem);
    for (vol = secBase->Volumes; vol; vol = vol->Next)
    {
        if (vol->FS_Flags && vol->FS_Name && !strcmp(vol->FS_Name, ia->volname))
        {
            D(bug(DEBUG_NAME_STR " %s: enforcer already running for '%s'\n", __func__, ia->volname);)
            goto fail;
        }
    }

    if ((fib = AllocDosObject(DOS_FIB, NULL)) && (RepPort = CreateMsgPort()) &&
        (secFSVolume = (struct secVolume *)MAlloc(sizeof(struct secVolume) + strlen(ia->volname) + 1)) &&
        (DevProc = GetDeviceProc(ia->volname, NULL)))
    {
        struct DosEnvec *de;
        struct FileSysStartupMsg *sm;
        struct DosList *dl, *dl2;

        /* Found the volume, verify that it is not multi-user aware already */
        if ((DevProc->dvp_DevNode->dol_Type == DLT_VOLUME || DevProc->dvp_DevNode->dol_Type == DLT_DEVICE) &&
            (sm = BADDR(DevProc->dvp_DevNode->dol_misc.dol_handler.dol_Startup)) && ((IPTR)sm > 1024) &&
            (de = BADDR(sm->fssm_Environ)) && (de->de_TableSize >= DE_DOSTYPE) &&
            (de->de_DosType != ID_muFS_DISK) && (de->de_DosType != ID_AFS_muFS_DISK) &&
            (de->de_DosType != ID_PFS2_muFS_DISK))
        {
            secFSVolume->DosList = DevProc->dvp_DevNode;
            secFSVolume->Process = &ipProc->pr_MsgPort;
            secFSVolume->OrigProc = DevProc->dvp_DevNode->dol_Task;
            secFSVolume->RepPort = RepPort;
            secFSVolume->fib = fib;
            secFSVolume->FS_Flags = ia->fs_flags;
            secFSVolume->LockCount = 0;
            secFSVolume->FS_Name = (STRPTR)(secFSVolume + 1);
            strcpy(secFSVolume->FS_Name, ia->volname);
            secFSVolume->RootOwner = ia->owner;
            secFSVolume->RootProtection = ia->protection;
            for (i = 0; i < TASKHASHVALUE; i++)
                NEWLIST((struct List *)&secFSVolume->ProxyHandles[i]);
            NEWLIST((struct List *)&secFSVolume->ProxyLocks);

            /* Determine the current volume for the filesystem */
            dl2 = (struct DosList *)BADDR(secFSDoPkt(secBase, secFSVolume, ACTION_CURRENT_VOLUME, 0, 0, 0, 0, 0, NULL));
            if (dl2 == NULL)
                dl2 = secFSVolume->DosList;

            if ((dl = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE)))
            {
                RemDosEntry(secFSVolume->DosList);
                if (dl2 != secFSVolume->DosList)
                    RemDosEntry(dl2);
                UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);
            }
            FreeDeviceProc(DevProc);
            DevProc = NULL;

            /* The target filesystem now has NO visible dos entries: insert ours */
            ConstructMountData(secBase, secFSVolume, (struct DeviceNode *)secFSVolume->DosList);
            ConstructVolumeData(secBase, secFSVolume, (struct DeviceList *)dl2);

            secFSVolume->Next = secBase->Volumes;
            secBase->Volumes = secFSVolume;
            ReleaseSemaphore(&secBase->VolumesSem);

            FreeV(ia);
            ipProc->pr_WindowPtr = winptrsave;

            D(bug(DEBUG_NAME_STR " %s: Intercepting IO for '%s'\n", __func__, secFSVolume->FS_Name);)
            InterceptorLoop(secBase, secFSVolume);
            /* NOTREACHED */
        }
    }

fail:
    ReleaseSemaphore(&secBase->VolumesSem);
    if (DevProc)
        FreeDeviceProc(DevProc);
    if (RepPort)
        DeleteMsgPort(RepPort);
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    Warn1(secBase, "Could not enforce volume \"%s\"", ia->volname);
    FreeV(ia);
    ipProc->pr_WindowPtr = winptrsave;
}

/*
 * Read the fstab and start an interceptor for every listed volume
 */
void ReadFSTab(struct SecurityBase *secBase)
{
    BPTR file, olddir;
    SIPTR *argarray[6];
    struct RDArgs *rdargs;
    int line;
    char *Buffer;

    if (!(secBase->Config.Flags & secCFGF_UseFSTab) || !secBase->_cfgLock || !ClearBuffer(secBase))
        return;
    Buffer = secBase->Buffer;

    D(bug(DEBUG_NAME_STR " %s: FSTAB enabled\n", __func__);)

    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
        olddir = CurrentDir(secBase->_cfgLock);
        if ((file = Open(secFSTab_FileName, MODE_OLDFILE)))
        {
            for (line = 1; FGets(file, Buffer, secGENBUFSIZE - 1); line++)
            {
                if (!Buffer[0] || Buffer[0] == '\n' || Buffer[0] == ';' || Buffer[0] == '#')
                    continue;
                rdargs->RDA_Source.CS_Buffer = Buffer;
                rdargs->RDA_Source.CS_Length = strlen(Buffer);
                rdargs->RDA_Source.CS_CurChr = 0;
                rdargs->RDA_DAList = 0;
                rdargs->RDA_Buffer = NULL;
                rdargs->RDA_BufSiz = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = RDAF_NOPROMPT;
                memset(argarray, 0, sizeof(argarray));

                if (ReadArgs(FSTAB_TEMPLATE, (SIPTR *)argarray, rdargs))
                {
                    if (argarray[argVOLNAME])
                    {
                        struct InterceptorArgs *ia;
                        ULONG namelen = strlen((STRPTR)argarray[argVOLNAME]) + 1;

                        if ((ia = MAllocV(sizeof(struct InterceptorArgs) + namelen)))
                        {
                            ia->secBase = secBase;
                            ia->volname = (STRPTR)(ia + 1);
                            CopyMem(argarray[argVOLNAME], ia->volname, namelen);
                            ia->fs_flags = secFSE_ENFORCED;
                            if (argarray[argREADONLY])
                                ia->fs_flags |= secFSE_READONLY;
                            if (argarray[argNOSUID])
                                ia->fs_flags |= secFSE_NOSUID;
                            ia->owner = secFSE_DEF_ROOTOWNER;
                            if (argarray[argROOTUID] && argarray[argROOTGID])
                                ia->owner = ((ULONG)*argarray[argROOTUID] << 16) | ((ULONG)*argarray[argROOTGID] & 0xffff);
                            ia->protection = secFSE_DEF_ROOTPROTECTION;
                            if (argarray[argROOTPROT])
                            {
                                LONG v;
                                if (StrToLong((STRPTR)argarray[argROOTPROT], &v) != -1)
                                    ia->protection = v;
                            }

                            if (!CreateNewProcTags(NP_Entry, (IPTR)InterceptorProcess,
                                                   NP_Name, (IPTR)"Security FS Enforcer",
                                                   NP_UserData, (IPTR)ia,
                                                   NP_Priority, 5,
                                                   TAG_DONE))
                            {
                                Warn1(secBase, "Could not start the enforcer for \"%s\"", ia->volname);
                                FreeV(ia);
                            }
                        }
                    }
                }
                else
                    Warn1(secBase, "Bad line in fstab: %ld", line);
                FreeArgs(rdargs);
            }
            Close(file);
        }
        else
            Warn0(secBase, "fstab enabled but no fstab file found");
        CurrentDir(olddir);
        FreeDosObject(DOS_RDARGS, rdargs);
    }
    else
        Die(secBase, NULL, AN_Unknown | AG_NoMemory);
}
