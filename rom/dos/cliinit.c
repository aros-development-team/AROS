/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <resources/filesysres.h>

#include "dos_intern.h"
    
static long internalBootCliHandler(void);

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH1(IPTR, CliInit,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, A0),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 154, Dos)

/*  FUNCTION

    Set up the first shell process. The DOS Packet can be NULL,
    in which case defaults are used.

    Currently, no DOS Packet arguments are used by this
    routine.

    A new Boot Cli is process is created, and 'dp' is
    sent to it. If the boot shell succeeds, then 'dp'
    is returned with dp_Res1 = DOSTRUE.
    has started.
    
    INPUTS

    dp  --  startup arguments specified as a packet

    RESULT

    RETURN_OK on success, ERROR_* (from dp_Res2) on failure.

    NOTES

    This function is internal to AROS, and should never be
    called by user space.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *mp, *reply_mp;
    struct DosPacket *my_dp = NULL;
    SIPTR Res1, Res2;
    BPTR seg;

    /* Create a DOS Process to handle this, since
     * we're probably being called from a Task context
     */
    if (dp == NULL) {
        /* This call does *not* require that
         * we be a DOS Process. Luckily.
         */
        my_dp = AllocDosObject(DOS_STDPKT, NULL);
        dp = my_dp;
    }

    if (dp == NULL)
        return ERROR_NO_FREE_STORE;
    
    reply_mp = CreateMsgPort();
    if (reply_mp == NULL) {
        if (my_dp)
            FreeDosObject(DOS_STDPKT, my_dp);
        return ERROR_NO_FREE_STORE;
    }

    seg = CreateSegList(internalBootCliHandler);

    mp = CreateProc("Boot Shell", 0, seg, AROS_STACKSIZE);
    if (mp == NULL) {
        DeleteMsgPort(reply_mp);
        if (my_dp)
            FreeDosObject(DOS_STDPKT, my_dp);
        /* A best guess... */
        UnLoadSeg(seg);
        return ERROR_NO_FREE_STORE;
    }

    /* Preload the reply with failure */
    dp->dp_Res1 = DOSFALSE;
    dp->dp_Res2 = ERROR_NOT_IMPLEMENTED;

    /* Again, doesn't require this Task to be a Process */
    SendPkt(dp, mp, reply_mp);

    /* Wait for the message from the Boot Cli */
    WaitPort(reply_mp);
    GetMsg(reply_mp);

    /* We know that if we're received a reply packet,
     * that we've been able to execute the handler,
     * therefore we can dispense with the 'CreateSegment'
     * stub.
     */
    UnLoadSeg(seg);

    Res1 = dp->dp_Res1;
    Res2 = dp->dp_Res2;

    if (my_dp)
        FreeDosObject(DOS_STDPKT, my_dp);

    DeleteMsgPort(reply_mp);

    D(bug("Dos/CliInit: Task returned Res1=%d, Res2=%d\n", (LONG)Res1, (LONG)Res2));

    /* Did we succeed? */
    if (Res1 == DOSTRUE)
        return RETURN_OK;

    return Res2;

    AROS_LIBFUNC_EXIT
} /* CliInit */

/* Find the most recent version of the matching filesystem
 */
static struct FileSysEntry *internalMatchFileSystemResourceHandler(struct FileSysResource *fsr, ULONG DosType)
{
    struct FileSysEntry *fse, *best_fse = NULL;

    ForeachNode(&fsr->fsr_FileSysEntries, fse) {
        if (fse->fse_DosType == DosType) {
            if (fse->fse_PatchFlags & (FSEF_HANDLER | FSEF_SEGLIST | FSEF_TASK)) {
                if (best_fse == NULL ||
                    fse->fse_Version > best_fse->fse_Version) {
                    best_fse = fse;
                }
            }
        }
    }
    D(bug("[Strap] Best fse for 0x%8x is: %p\n", DosType, best_fse));

    return best_fse;
}


/* See if the BootNode's DeviceNode needs to be patched by
 * an entry in FileSysResource
 *
 * If de->de_DosType == 0, and no dn_SegList nor dn_Handler,
 * then the node uses the 'defseg' handler.
 */
static void internalPatchBootNode(struct FileSysResource *fsr, struct BootNode *bn, BPTR defseg)
{
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;
    struct FileSysEntry *fse;

    dn = bn->bn_DeviceNode;
    if (!dn)
        return;

    /* If we already have a task installed,
     * then we're done.
     */
    if (dn->dn_Task != NULL)
        return;

    /* If we already have a handler installed,
     * then we're done.
     */
    if (dn->dn_SegList != BNULL)
        return;

    fssm = BADDR(dn->dn_Startup);
    if (fssm == NULL)
        return;

    de = BADDR(fssm->fssm_Environ);
    if (de == NULL)
        return;

    /* If the DosType is 0 and dn_Handler == BNULL, use the default handler */
    if (de->de_DosType == 0 && dn->dn_Handler == BNULL) {
        dn->dn_SegList = defseg;
        return;
    }

    /* If no FileSysResource, nothing to do */
    if (fsr == NULL) {
        D(bug("[CliInit] No FileSystem.resource, not patching DeviceNode %p\n", dn));
        return;
    }

    D(bug("[Boot] Looking for patches for DeviceNode %p\n", dn));

    /*
     * internalMatchFileSystemResourceHandler looks up the filesystem
     */
    fse = internalMatchFileSystemResourceHandler(fsr, de->de_DosType);
    if (fse != NULL)
    {
        D(bug("[Boot] found 0x%08x in FileSystem.resource\n", de->de_DosType));
        dn->dn_SegList = fse->fse_SegList;
        /* other fse_PatchFlags bits are quite pointless */
        if (fse->fse_PatchFlags & FSEF_TASK)
            dn->dn_Task = (APTR)fse->fse_Task;
        if (fse->fse_PatchFlags & FSEF_LOCK)
            dn->dn_Lock = fse->fse_Lock;

        /* Adjust the stack size for 64-bits if needed.
         */
        if (fse->fse_PatchFlags & FSEF_STACKSIZE)
            dn->dn_StackSize = (fse->fse_StackSize/sizeof(ULONG))*sizeof(IPTR);
        if (fse->fse_PatchFlags & FSEF_PRIORITY)
            dn->dn_Priority = fse->fse_Priority;
        if (fse->fse_PatchFlags & FSEF_GLOBALVEC)
            dn->dn_GlobalVec = fse->fse_GlobalVec;
    }
}

static BPTR internalBootLock(struct DosLibrary *DOSBase, struct FileSysResource *fsr)
{
    struct ExpansionBase *ExpansionBase;
    struct BootNode *bn;
    struct DeviceNode *dn;
    BPTR lock = BNULL;
    STRPTR name;
    int name_len;

    /* The first BootNode off of the MountList.
     * The dosboot.resource strap routine will have
     * made the desired boot node the first in this list.
     *
     * If this fails to mount, we will fail, which will
     * cause dos.library to fail to initialize, and 
     * then dosboot.resource will handle checking the
     * next device in the list.
     */
    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (!ExpansionBase)
        return BNULL;

    bn = (struct BootNode *)GetHead(&ExpansionBase->MountList);
    CloseLibrary((APTR)ExpansionBase);

    if (bn == NULL)
        return BNULL;

    dn = bn->bn_DeviceNode;
    if (dn == NULL || dn->dn_Name == BNULL)
        return BNULL;

    /* Patch it up, if needed */
    internalPatchBootNode(fsr, bn, DOSBase->dl_Root->rn_FileHandlerSegment);

     D(bug("Dos/CliInit: %b (%d) appears usable\n", dn->dn_Name, bn->bn_Node.ln_Pri));

    /* Try to find a Lock for 'name:' */
    name_len = AROS_BSTR_strlen(dn->dn_Name);
    name = AllocVec(name_len + 2, MEMF_ANY);
    if (name != NULL) {
        struct DosList *dl;
        LONG ret;

        CopyMem(AROS_BSTR_ADDR(dn->dn_Name),name,name_len);
        name[name_len+0] = 0;

        /* Check the DOS list for a volume of the same name */
        dl = LockDosList(LDF_ALL | LDF_READ);
        dl = FindDosEntry(dl, name, LDF_ALL);
        UnLockDosList(LDF_ALL | LDF_READ);

        /* Not in the DOS mount list? Add it. */
        if (dl == NULL) {
            ret = AddDosEntry((struct DosList *)dn);
            if (ret == DOSFALSE) {
                /* Duplicate name? Something's wrong. 
                 */
                D(bug("Dos/CliInit: Can't add %s to the DOS device list\n", name));
                FreeVec(name);
                return BNULL;
            }
        }

        /* Make the volume name a volume: name */
        name[name_len+0] = ':';
        name[name_len+1] = 0;
        D(bug("Dos/CliInit:   Attempt to Lock(\"%s\")...\n", name, BADDR(lock)));
        lock = Lock(name, SHARED_LOCK);
        D(bug("Dos/CliInit:   Lock(\"%s\") => %p\n", name, BADDR(lock)));

        if (lock == BNULL) {
            RemDosEntry((struct DosList *)dn);
        } else {
            BOOL bootable;

            /* If we have a lock, check the per-platform
             * conditional boot code.
             */
            /* Temporarily set dl_SYSLock for the following routine */
            DOSBase->dl_SYSLock = lock;
            bootable = __dos_IsBootable(DOSBase, lock);
            DOSBase->dl_SYSLock = BNULL;

            if (!bootable) {
                struct MsgPort *mp;
                SIPTR err;

                /* Darn. Not bootable. Try to unmount it. */
                D(bug("Dos/CliInit:   Does not have a bootable filesystem\n"));

                /* First, get the lock's handler */
                mp = ((struct FileLock *)BADDR(lock))->fl_Task;
                /* Unlock the lock. */
                UnLock(lock);
                lock = BNULL;

                /* Then try to ACTION_DIE the handler. */
                err = DoPkt(mp, ACTION_DIE, 0, 0, 0, 0, 0);
                if (err != DOSTRUE) {
                    /* Well, we can't unmount it, so let's
                     * just continue on.
                     */
                    D(bug("Dos/CliInit:   Cannot be unmounted!\n"));
                }

                /* Excellent, it unmounted, we'll try again later.
                 */
            }
        }

        FreeVec(name);
    }

    /* If we have a valid lock, the node was usable,
     * so remove it from the MountList, since it has
     * already been mounted.
     */
    if (lock != BNULL)
        Remove(&bn->bn_Node);

    return lock;
}

static void AddBootAssign(CONST_STRPTR path, CONST_STRPTR assign, APTR DOSBase)
{
    BPTR lock;
    if (!(lock = Lock(path, SHARED_LOCK)))
    	lock = Lock("SYS:", SHARED_LOCK);
    if (lock)
        AssignLock(assign, lock);
}


/* This is what actually gets the Lock() for SYS:,
 * sets up the boot assigns, and starts the
 * startup sequence.
 */
static long internalBootCliHandler(void)
{
    struct ExpansionBase *ExpansionBase;
    struct DosLibrary *DOSBase;
    struct MsgPort *mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    BPTR lock;
    struct DosPacket *dp;
    IPTR BootFlags = 0;
    struct BootNode *bn, *tmpbn;
    struct FileSysResource *fsr;

    /* Ah. A DOS Process context. At last! */
    WaitPort(mp);
    dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (DOSBase == NULL) {
        D(bug("Dos/CliInit: Impossible! Where did dos.library go?\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AO_DOSLib);
    }

    /* It's perfectly fine if this fails. */
    fsr = OpenResource("FileSystem.resource");

    /* Find and Lock the proposed boot device */
    lock = internalBootLock(DOSBase, fsr);
    D(bug("Dos/CliInit: Proposed SYS: lock is: %p\n", BADDR(lock)));

    /* Ok, at this point we've either succeeded or failed.
     *
     * Inform our parent.
     */
    ReplyPkt(dp, (lock == BNULL) ? DOSFALSE : DOSTRUE, IoErr());
    /* We've replied, don't touch dp anymore! */
    D(dp = NULL; (void)dp;);

    if (lock == BNULL)
        return IoErr();

    /* We're now at the point of no return. */
    DOSBase->dl_SYSLock = DupLock(lock);
    DOSBase->dl_Root->rn_BootProc = ((struct FileLock*)BADDR(lock))->fl_Task;
    SetFileSysTask(DOSBase->dl_Root->rn_BootProc);

    AssignLock("SYS", lock);
    lock = Lock("SYS:", SHARED_LOCK);
    if (lock == BNULL) {
        D(bug("DOS/CliInit: Impossible! The SYS: assign failed!\n"));
        Alert(AT_DeadEnd | AG_BadParm | AN_DOSLib);
    }

    AddBootAssign("SYS:C",                "C", DOSBase);
    AddBootAssign("SYS:Libs",             "LIBS", DOSBase);
    AddBootAssign("SYS:Devs",             "DEVS", DOSBase);
    AddBootAssign("SYS:L",                "L", DOSBase);
    AddBootAssign("SYS:S",                "S", DOSBase);
    AddBootAssign("SYS:Fonts",            "FONTS", DOSBase);

#if !(mc68000)
    /* Let hidds in DRIVERS: directory be found by OpenLibrary */
    if ((lock = Lock("DEVS:Drivers", SHARED_LOCK)) != BNULL) {
        AssignLock("DRIVERS", lock);
        AssignAdd("LIBS", lock);
    }
#endif

    AssignLate("ENV", "SYS:Prefs/Env-Archive");
    AssignLate("ENVARC", "SYS:Prefs/Env-Archive");

    /*
     * Retry to run handlers which were not started yet.
     * Here we already can load disk-based handlers.
     */
    D(bug("Dos/CliInit: Assigns done, retrying mount handlers\n"));
    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (ExpansionBase) {
        BootFlags = ExpansionBase->eb_BootFlags;
        D(bug("Dos/CliInit: Using Boot Flags of 0x%x\n", BootFlags));

        ForeachNodeSafe(&ExpansionBase->MountList, bn, tmpbn) {
            struct DeviceNode *dn = bn->bn_DeviceNode;

            if (dn == NULL || dn->dn_Name == BNULL)
                continue;

            /* Patch it up, if needed */
            internalPatchBootNode(fsr, bn, DOSBase->dl_Root->rn_FileHandlerSegment);

            if (AddDosEntry((struct DosList *)dn) != DOSFALSE) {
                D(bug("Dos/CliInit: Added %b: to the DOS list\n", dn->dn_Name));
                if (bn->bn_Flags & ADNF_STARTPROC) {
                    D(bug("Dos/CliInit: Mounting %b: ", dn->dn_Name));
                    RunHandler(dn, NULL, DOSBase);
                    D(bug("dn->dn_Task = %p\n", dn->dn_Task));
                }
            }
        }
        CloseLibrary((APTR)ExpansionBase);
    }

    /* Init all the RTF_AFTERDOS code, since we now have
     * SYS:, the dos devices, and all the other assigns.
     */
    D(bug("Dos/CliInit: Calling InitCode(RTF_AFTERDOS, 0)\n"));
    InitCode(RTF_AFTERDOS, 0);

    /* We will now officially set the current directory to SYS: */
    CurrentDir(DOSBase->dl_SYSLock);

    /* Call the platform-overridable portions */
    D(bug("Dos/CliInit: Calling __dos_Boot(%p, 0x%x)\n", DOSBase, BootFlags));
    __dos_Boot(DOSBase, BootFlags);

    D(bug("Dos/CliInit: Boot sequence exited\n"));
    CloseLibrary((APTR)DOSBase);

    /* And exit... */
    return RETURN_OK;
}
