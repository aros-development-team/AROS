
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_memory.h"
#include "security_support.h"

/*  */
#define FSTAB_TEMPLATE \
	"VOLUME/A,ROOTPROTECTION/K,ROOTUID/K/N,ROOTGID/K/N,READONLY=RO/K/S,NOSUID/K/S"


#define argVOLNAME	0
#define argROOTPROT	1
#define argROOTUID	2
#define argROOTGID	3
#define argREADONLY	4
#define argNOSUID       5


/* Despatch Table */

struct secFSE_PktHandler despatch[] = {
#if (0)
	/* Fully Implemented */
	{ ACTION_DISK_INFO, 			PKTHANDLER(DiskInfo) }, /* Most Used */
	{ ACTION_LOCATE_OBJECT, 	PKTHANDLER(LocateObject) },
	{ ACTION_COPY_DIR, 			PKTHANDLER(CopyDir) },
	{ ACTION_FREE_LOCK, 			PKTHANDLER(FreeLock) },
	{ ACTION_EXAMINE_OBJECT,			PKTHANDLER(ExamineObject) },
	{ ACTION_EXAMINE_NEXT,			PKTHANDLER(ExamineNext) },
	{ ACTION_CREATE_DIR, 		PKTHANDLER(CreateDir) },
	{ ACTION_DELETE_OBJECT, 	PKTHANDLER(DeleteObject) },
	{ ACTION_RENAME_OBJECT, 	PKTHANDLER(RenameObject) },
	{ ACTION_SAME_LOCK,			PKTHANDLER(SameLock) },
	{ ACTION_PARENT, 				PKTHANDLER(Parent) },
	{ ACTION_SET_PROTECT, 		PKTHANDLER(SetProtect) },
	{ ACTION_SET_COMMENT, 		PKTHANDLER(SetComment) },
	{ ACTION_SET_DATE, 			PKTHANDLER(SetDate) },
	{ ACTION_FH_FROM_LOCK, 		PKTHANDLER(FHFromLock) },
	{ ACTION_MAKE_LINK, 			PKTHANDLER(MakeLink) },
	{ ACTION_READ_LINK,			PKTHANDLER(ReadLink) },
	{ ACTION_CHANGE_MODE,		PKTHANDLER(ChangeMode) },
	{ ACTION_COPY_DIR_FH,		PKTHANDLER(CopyDirFH) },
	{ ACTION_PARENT_FH,		PKTHANDLER(ParentFH) },
	{ ACTION_EXAMINE_ALL,		PKTHANDLER(ExamineAll) },
	{ ACTION_CURRENT_VOLUME,	PKTHANDLER(CurrentVolume) },
	{ ACTION_SET_OWNER, 			PKTHANDLER(SetOwner) },
	
	{ ACTION_EXAMINE_FH,			PKTHANDLER(ExamineFH) },

	
	{ ACTION_FINDINPUT, 			PKTHANDLER(FindInput) },
	{ ACTION_FINDOUTPUT, 		PKTHANDLER(FindOutput) },
	{ ACTION_FINDUPDATE, 		PKTHANDLER(FindUpdate) },
	{ ACTION_END, 					PKTHANDLER(End) },
	{ ACTION_READ, 				PKTHANDLER(Read) },
	{ ACTION_WRITE, 				PKTHANDLER(Write) },
	{ ACTION_INFO, 				PKTHANDLER(Info) },
	{ ACTION_INHIBIT, 			PKTHANDLER(Inhibit) },
	{ ACTION_DIE, 					PKTHANDLER(Die) },

	/* To be implemented */

	{ ACTION_FLUSH,				PKTHANDLER(Flush) },
	{ ACTION_SEEK, 				PKTHANDLER(Seek) },
	{ ACTION_SET_FILE_SIZE, 		PKTHANDLER(SetFileSize) },
	{ ACTION_RENAME_DISK, 		PKTHANDLER(RenameDisk) },
	{ ACTION_FORMAT, 				PKTHANDLER(Format) },
	{ ACTION_MORE_CACHE, 		PKTHANDLER(MoreCache) },
	{ ACTION_WRITE_PROTECT, 	PKTHANDLER(WriteProtect) },
	{ ACTION_ADD_NOTIFY,			PKTHANDLER(AddNotify) },
	{ ACTION_REMOVE_NOTIFY,			PKTHANDLER(RemoveNotify) },
	{ ACTION_FREE_RECORD,			PKTHANDLER(FreeRecord) },
	{ ACTION_LOCK_RECORD,			PKTHANDLER(LockRecord) },
	/* End of Table */
#endif
	{ 0, NULL }
};


/* Make sure the DosList and LockList point to us */

void EnforceLists(struct secVolume *Vol)
{
	struct DosList *dl;
	int modcount = 0;
	D(bug( DEBUG_NAME_STR " %s()\n", __func__));
	dl = LockDosList(LDF_VOLUMES|LDF_DEVICES|LDF_WRITE);
	while((dl = NextDosEntry(dl, LDF_VOLUMES|LDF_DEVICES|LDF_WRITE)))	{
            if (dl->dol_Task == Vol->OrigProc){
                //dl->dol_Task = Vol->Process;
                modcount++;
#if 0
                if (dl->dol_Type == DLT_VOLUME)	{
                    /* Walk the list of locks */
                    struct FileLock *fl;
                    fl = BADDR(dl->dol_misc.dol_volume.dol_LockList);
                    while(fl)	{
                        if (fl->fl_Task == Vol->OrigProc)
                                fl->fl_Task = Vol->Process;
#ifdef DEBUG
                        if (BADDR(fl->fl_Volume) != dl)
                                kprintf("INSANITY: Volume not correct for lock!\n");
#endif
                        fl = BADDR(fl->fl_Link);
                    }
                }
#endif
            }
	}
	UnLockDosList(LDF_VOLUMES|LDF_WRITE);
	D(bug( DEBUG_NAME_STR " %s: Modified %d entries\n", __func__, modcount));
}

/* Construct the bits required to mount the enforcer */
static int ConstructMountData(struct secVolume * Vol, struct DeviceNode * dn2)
{
    char devname[256];
    char *bstr;
    int i, len;
    struct DeviceNode * dn;

    bstr = BADDR(dn2->dn_Name);
    len = *bstr++;
    /* copy name of device */
    for (i=0; i<len; i++)
        devname[i] = bstr[i];
    devname[i] = 0;
            
    dn = (struct DeviceNode*)MakeDosEntry(devname, DLT_DEVICE);
    if (dn)	{
        dn->dn_Task = Vol->Process;
        dn->dn_SegList = (BPTR)-1;
        dn->dn_GlobalVec = (BPTR)-1;
        Vol->ProxyDosList = dn;
        AddDosEntry((struct DosList*)dn);
        return TRUE;
    }
    D(bug( DEBUG_NAME_STR " %s: failed!\n", __func__));

    return FALSE;
}

static int ConstructVolumeData(struct secVolume * Vol, struct DeviceList * dl2)
{
    char volname[256];
    char * bstr;
    int i, len;
    struct DeviceList * dl;

    bstr = BADDR(dl2->dl_Name);
    len = *bstr++;
    /* copy name of volume */
    for (i=0; i<len; i++)
        volname[i] = bstr[i];
    volname[i] = 0;

    dl = (struct DeviceList*)MakeDosEntry(volname, DLT_VOLUME);
    if (dl)	{
        dl->dl_Task = Vol->Process;
        dl->dl_DiskType = dl2->dl_DiskType;

        Vol->ProxyDosListVolume = dl;
        AddDosEntry((struct DosList*)dl);
        return TRUE;
    }
    D(bug( DEBUG_NAME_STR " %s: failed!\n", __func__));

    return FALSE;
}

/* The Packet Intercepting/Relaying Loop */

static void InterceptorLoop(struct SecurityBase *secBase, struct secVolume * iVol)
{
    struct secExtOwner * owner;
    BOOL handled;
    LONG action,i;
    struct Message *mes;
    struct DosPacket *pkt;
    struct MsgPort *ivPort = iVol->Process;

    while(TRUE)
    {
        //D(kprintf("Interceptor: Waiting for msg\n"));
        while((mes = GetMsg(ivPort))==NULL)
        {
            Wait(1<<ivPort->mp_SigBit);
        }
        //D(kprintf("Interceptor: Got a message\n"));
        pkt = (struct DosPacket*)mes->mn_Node.ln_Name;
        action = pkt->dp_Type;
        owner = GetPktOwner(secBase, pkt);

        /* Intercept and handle here */
        i=0;
        handled = FALSE;
        D(
            bug( DEBUG_NAME_STR " %s: ACTION: %ld from: [%08lx|%p]\n", __func__, iVol->FS_Name ,action, secExtOwner2ULONG(owner), GetPktTask(pkt));
            if (GetPktTask(pkt) == FindTask(NULL))
                    bug( DEBUG_NAME_STR " %s:     WARNING: packet from myself!!??\n", __func__);
          )

        while(despatch[i].action != 0)
        {
            if ((despatch[i].action == action) && despatch[i].func )
            {
                handled = despatch[i].func(iVol, pkt, owner);
                break;
            }
            i++;
        }
        if (!handled) {
#ifdef DEBUG
            if ((despatch[i].action == action) && despatch[i].func )
                kprintf("\n");
#endif
            DoPacket(iVol, pkt);
        }
        D(bug( DEBUG_NAME_STR " %s:     RESULT: 1:%lx 2:%lx\n", __func__, pkt->dp_Res1, pkt->dp_Res2));
        ReplyPkt(pkt, pkt->dp_Res1, pkt->dp_Res2);

        if (owner)
            secFreeExtOwner(owner);
    }
}

/* Interceptor Code */

/* The interceptor process is started and given a single argument; the line
 * from the FSTAB, which is parsed and the process then attempts to find the
 * controling process of that volume and hijack it */

static int InterceptorProcess(void)
{
    struct SecurityBase *secBase;
    struct FileInfoBlock *fib;
    struct MsgPort *RepPort;	/* For Talking to the real FS */
    struct secVolume *secFSVolume;
    struct DevProc *DevProc;
    struct Process *ipProc;
    static SIPTR argarray[6] = { 0, 0, 0, 0, 0, 0};
    struct RDArgs *rdargs;
    APTR winptrsave;
    LONG fs_flags;
    LONG owner, protection;
    BOOL havesem = FALSE;

#define GETSEM()	{ObtainSemaphore(&secBase->VolumesSem), havesem = TRUE;}
#define RELSEM()	{if (havesem)	ReleaseSemaphore(&secBase->VolumesSem);}
#define GO(a)		{ipProc->pr_WindowPtr = winptrsave; return a; }

    ipProc = (struct Process*)FindTask(NULL);
    winptrsave = ipProc->pr_WindowPtr;
    ipProc->pr_WindowPtr = (APTR) -1;

    secBase = (struct SecurityBase *)ipProc->pr_Task.tc_UserData;
    D(bug( DEBUG_NAME_STR " %s: secBase @ %p\n", __func__, secBase);)

    /* XXX: Make sure that we are not leaking memory from ReadArgs */
    if ((rdargs = ReadArgs(FSTAB_TEMPLATE, argarray, NULL))==NULL)
    {
        /* This is insane; the args should be valid, since we only start this
         * process if they are! */
        GO(20);
    }

    D(bug( DEBUG_NAME_STR " %s: secFS Enforcer %p %s Starting Up\n", __func__, ipProc, argarray[argVOLNAME]));

    /* Make sure we aren't being re-run */
    GETSEM();
    for(secFSVolume = secBase->Volumes; secFSVolume; secFSVolume = secFSVolume->Next)
    {
        if (secFSVolume->FS_Flags)
        {
            if (secFSVolume->FS_Name && (strcmp(secFSVolume->FS_Name,(STRPTR)argarray[argVOLNAME])==0))
            {
                D(bug( DEBUG_NAME_STR " %s: NOT running ; secFS already running!\n", __func__));
                RELSEM();
                GO(20);
            }
        }
    }
    fs_flags = secFSE_ENFORCED;
    if (argarray[argREADONLY])
        fs_flags |= secFSE_READONLY;
    if (argarray[argNOSUID])
        fs_flags |= secFSE_NOSUID;

    owner = secFSE_DEF_ROOTOWNER;
#if 0
    if (argarray[argROOTUID] && argarray[argROOTGID])
        owner = (argarray[argROOTUID] << 16) | argarray[argROOTGID];
#endif
    protection = secFSE_DEF_ROOTPROTECTION;

    if (argarray[argROOTPROT])
    {
        /* Parse the Protection Arg Here */
    }

    secFSVolume = NULL;

    if ( (fib = AllocDosObject(DOS_FIB, NULL)) &&
        (RepPort = CreateMsgPort()) &&
        (secFSVolume = (struct secVolume*)MAlloc(sizeof(struct secVolume)))
        )
    {
        if ((DevProc = GetDeviceProc((STRPTR)argarray[argVOLNAME], NULL)))
        {
            struct DosEnvec *de;
            struct FileSysStartupMsg *sm;

            /* Found the volume, verify that we can use it */
            if ((DevProc->dvp_DevNode->dol_Type == DLT_VOLUME || 
                            DevProc->dvp_DevNode->dol_Type == DLT_DEVICE) &&
                            (sm = BADDR(DevProc->dvp_DevNode->dol_misc.dol_handler.dol_Startup)) &&
                            (de = BADDR(sm->fssm_Environ)) && (de->de_TableSize >= DE_DOSTYPE) &&
                            (de->de_DosType != ID_muFS_DISK) &&
                            (de->de_DosType != ID_AFS_muFS_DISK) &&
                            (de->de_DosType != ID_PFS2_muFS_DISK))
            {
                struct DosList *dl, *dl2;

                /* NOTE NOTE NOTE:
                 * We need to make sure that we get both the DEVICE and VOLUME nodes for
                 * the given volume; this means that the filesystem is mounted.
                 */

                /* OK to Enforce this FS */
                secFSVolume->DosList = DevProc->dvp_DevNode;
                secFSVolume->Process = &ipProc->pr_MsgPort;
                secFSVolume->OrigProc = DevProc->dvp_DevNode->dol_Task;
                D(bug( DEBUG_NAME_STR " %s: OrigProc is %p\n", __func__, secFSVolume->OrigProc));
                secFSVolume->RepPort = RepPort;
                secFSVolume->fib = fib;
                secFSVolume->FS_Flags = fs_flags;
                secFSVolume->LockCount = 0;
                /* Allocate a copy of the name */
                secFSVolume->FS_Name = (STRPTR)argarray[argVOLNAME];
                secFSVolume->RootOwner = owner;
                secFSVolume->RootProtection = protection;

                /* Initialize FH Cache */

                {
                    register int i;
                    for (i=0; i < TASKHASHVALUE; i++)	{
                        NewList((struct List*)&secFSVolume->FHCache[i]);
                        NewList((struct List*)&secFSVolume->ProxyHandles[i]);
                    }
                }
                NewList((struct List*)&secFSVolume->ProxyLocks);

                D(bug( DEBUG_NAME_STR " %s: Hijacking Filesystems ...\n", __func__);)

                /* Determine the current volume for the filesystem */
                dl2 = (struct DosList*)BADDR(secFSDoPkt(secFSVolume, ACTION_CURRENT_VOLUME, 0,0,0,0,0));
                if (dl2 == NULL)	{
                    D(bug( DEBUG_NAME_STR " %s: WARNING: volume is not mounted!\n", __func__));
                    dl2 = secFSVolume->DosList;
                }

                if ((dl = LockDosList(LDF_VOLUMES|LDF_DEVICES|LDF_WRITE)))
                {
                    RemDosEntry(secFSVolume->DosList);
                    if (dl2 != NULL && dl2 != secFSVolume->DosList)
                            RemDosEntry(dl2);

                    UnLockDosList(LDF_VOLUMES|LDF_WRITE);
                }
                /* The target filesystem now has NO visible dos entries! */

                /* Insert our details */
                ConstructMountData(secFSVolume, (struct DeviceNode*)secFSVolume->DosList);
                ConstructVolumeData(secFSVolume, (struct DeviceList*)dl2);

                D(bug( DEBUG_NAME_STR " %s: Enforcing Filesystems ...\n", __func__);)

                EnforceLists(secFSVolume);

                /* Add secFSVolume to the list in the library */

                secFSVolume->Next = secBase->Volumes;
                secBase->Volumes = secFSVolume;
                RELSEM();

                /* Ready to Enforce ! */

                D(bug( DEBUG_NAME_STR " %s: Intercepting IO ...\n", __func__);)

                InterceptorLoop(secBase, secFSVolume);

                /* NOTREACHED */
            }
        }
    }
    /* Warn user, or Die ? 
* Die is probably the best idea in a release version */

    /* NOT REACHED */
    RELSEM();
    bug(DEBUG_NAME_STR "Err, got to end of Interceptor Process?\n");
    GO(20);	/* Must have been an error to get here! */
}


/* Find the rendevous port and collect config and passwd dir information from
 * it */

void ReadFSTab(struct SecurityBase *secBase)
{
    BPTR file;
    SIPTR *argarray[6];

    struct RDArgs *rdargs;
    int line;
    char Buffer[256];

    D(bug( DEBUG_NAME_STR " %s()\n", __func__));

    /* Dont bother if we dont have to read it */
    
    if ((secBase->Config.Flags & muCFGF_UseFSTab)==FALSE) {
            D(bug( DEBUG_NAME_STR " %s: FSTAB not enabled\n", __func__));
            return;
    }
    D(bug( DEBUG_NAME_STR " %s: FSTAB enabled!\n", __func__));
    
    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL))) {
        CurrentDir(secBase->_cfgLock);
        if ((file = Open(secFSE_FSTAB_FILENAME, MODE_OLDFILE))) {
            for (line=1; FGets(file, Buffer, sizeof(Buffer)-1); line++) {
                rdargs->RDA_Source.CS_Buffer = Buffer;
                rdargs->RDA_Source.CS_Length = strlen(Buffer);
                rdargs->RDA_Source.CS_CurChr = 0;
                rdargs->RDA_DAList = 0;
                rdargs->RDA_Buffer = NULL;
                rdargs->RDA_BufSiz = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = RDAF_NOPROMPT;
                memset(argarray, 0, sizeof(argarray));

                if (ReadArgs(FSTAB_TEMPLATE, (SIPTR*)argarray, rdargs)) {
                    if (argarray[argVOLNAME]) {
                        struct Process *child;
                        D(bug( DEBUG_NAME_STR " %s: FSTAB volume %s\n", __func__, argarray[argVOLNAME]));

                        child = CreateNewProcTags(NP_Entry, (IPTR)InterceptorProcess,
                                    NP_Name, "Heimdall FS Enforcer Process",
                                    NP_UserData,  (IPTR)secBase,
                                    NP_Arguments, argarray[argVOLNAME],
                                    TAG_DONE);

                        if (child==NULL)	{
                            Warn(secBase, "CreateNewProc FAILED for Heimdall FS Enforcer Process\n");
                            D(bug( DEBUG_NAME_STR " %s: FAILED: Heimdall FS Enforcer: Cant CreateNewProc!\n", __func__));
                            /* XXX: We should probably die here */
                        }
                    }
                } else {
                    Warn(secBase, "Bad line in fstab; %ld\n", line);
                }
                FreeArgs(rdargs);
            }
            Close(file);
        } else
            Warn(secBase, "No fstab!\n");
        FreeDosObject(DOS_RDARGS, rdargs);
    } else
        Die(NULL, AN_Unknown | AG_NoMemory);
}

BOOL BootStrapRendevous(struct SecurityBase *secBase)
{
    struct secFSEnforceRendevous *rndv;
    if ((rndv = (struct secFSEnforceRendevous*)FindPort(MUFS_ENFORCE_PORTNAME)))
    {
        /* Bootstrap Present */

        secBase->_pwdLock = Lock(rndv->PasswdDir, ACCESS_READ);
        secBase->_cfgLock = Lock(rndv->ConfigDir, ACCESS_READ);

        if (secBase->_pwdLock && secBase->_cfgLock)
            return TRUE;
    }
    return FALSE;
}

/*	Access Control
 *
 *	This decides if we should grant access priveleges to someone
 *	*/

LONG IsAllowed(struct SecurityBase *secBase, struct secVolume *Vol, struct secExtOwner *task, 
		ULONG object, LONG prot, LONG access)
{
    LONG retval = 0;
    ULONG who = secGetRelationshipA(task, object, NULL);
    D(LONG a = secExtOwner2ULONG(task));
    D(bug( DEBUG_NAME_STR " %s: struct secExtOwner *task == %p\n", __func__, task));

    /* Alter the effective prot if we are read-only */

    D(bug( DEBUG_NAME_STR " %s: Object owned by %08lx, Task is %08lx\n", __func__, object, a));

    if (Vol != NULL && Vol->FS_Flags & secFSE_READONLY)
    {
        /* Remove all write flags */
        D(bug( DEBUG_NAME_STR " %s: FS is READ-ONLY; removing WRITE permissions from protection\n", __func__));
        prot &= ~((!FIBF_WRITE)|FIBF_GRP_WRITE|FIBF_OTR_WRITE);
    }

    /* Root is always allowed */
    if (who & secRelF_ROOT_UID)
    {
        if ((Vol != NULL) && (Vol->FS_Flags & secFSE_READONLY) && (access & secAt_Write))
        {
            /* Read-Only FileSystem */
            D(bug( DEBUG_NAME_STR " %s: SuperUser denied Write access - READ-ONLY FS\n", __func__));
            return secAC_PERMISSION_DENIED|secAC_READ_ONLY_FS|secAC_ROOT_DENIED;
        }
        D(bug( DEBUG_NAME_STR " %s: SuperUser - Access Granted\n", __func__));
        return secAC_PERMISSION_GRANTED;
    }
    if (who & secRelF_UID_MATCH)
    {
        D(bug( DEBUG_NAME_STR " %s: Task is object owner\n", __func__));
        if ((access == (secAt_Read|secAt_Write) && !(prot & (FIBF_READ|FIBF_WRITE))))
            return secAC_PERMISSION_GRANTED; /* Allowed RW */
        else if ((access == secAt_Read) && !(prot & FIBF_READ))
            return secAC_PERMISSION_GRANTED; /* Allowed R */
        else if ((access == secAt_Write) && !(prot & FIBF_WRITE))
            return secAC_PERMISSION_GRANTED; /* Allowed W */
        else if ((access == secAt_Delete) && !(prot & FIBF_DELETE))
            return secAC_PERMISSION_GRANTED; /* Allowed D */
        retval |= secAC_OWNER_DENIED;
    }
    if (who & secRelF_GID_MATCH)
    {
        D(bug( DEBUG_NAME_STR " %s: Task is in same group as object\n", __func__));
        if ((access == (secAt_Read|secAt_Write) && (prot & (FIBF_GRP_READ|FIBF_GRP_WRITE))))
            return secAC_PERMISSION_GRANTED;	/* RW */
        else if ((access == secAt_Read) && (prot & FIBF_GRP_READ))
            return secAC_PERMISSION_GRANTED;	/* R */
        else if ((access == secAt_Write) && (prot & FIBF_GRP_WRITE))
            return secAC_PERMISSION_GRANTED;	/* W */
        else if ((access == secAt_Delete) && (prot & FIBF_GRP_DELETE))
            return secAC_PERMISSION_GRANTED;	/* D */
        retval |= secAC_GROUP_DENIED;
    }
    D(else
        bug( DEBUG_NAME_STR " %s: Task has no special relation with object\n", __func__);
    )
    if ((access == (secAt_Read|secAt_Write) && (prot & (FIBF_OTR_READ|FIBF_OTR_WRITE))))
        return secAC_PERMISSION_GRANTED;	/* RW */
    else if ((access == secAt_Read) && (prot & FIBF_OTR_READ))
        return secAC_PERMISSION_GRANTED;	/* R */
    else if ((access == secAt_Write) && (prot & FIBF_OTR_WRITE))
        return secAC_PERMISSION_GRANTED;	/* W */
    else if ((access == secAt_Delete) && (prot & FIBF_OTR_DELETE))
        return secAC_PERMISSION_GRANTED;	/* D */

    D(bug( DEBUG_NAME_STR " %s: ACCESS DENIED\n", __func__));
    return secAC_PERMISSION_DENIED | retval;	/* No Access */
}
