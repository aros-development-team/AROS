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

#include <ctype.h>

#include "dos_intern.h"

#ifdef DEBUG_DOSTYPE

static void PRINT_DOSTYPE(ULONG dt)
{
    unsigned int i;

    bug("Dos/CliInit: DosType is ");

    for (i = 0; i < 4; i++)
    {
    	unsigned char c = dt >> (24 - i * 8);

	if (isprint(c))
    	    RawPutChar(c);
    	else
    	    bug("\\%02X", c);
    }

    RawPutChar('\n');
}

#else

#define PRINT_DOSTYPE(dt)

#endif

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

    Set up the first shell process.

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

    mp = CreateProc("Boot Mount", 0, seg, AROS_STACKSIZE);
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

    D(bug("Dos/CliInit: Task returned Res1=%ld, Res2=%ld\n", Res1, Res2));

    /* Did we succeed? */
    if (Res1 == DOSTRUE)
        return RETURN_OK;
    /* Make sure we return non-zero error code, 0 == RETURN_OK */
    if (Res2 == 0)
    	Res2 = ERROR_UNKNOWN;

    return Res2;

    AROS_LIBFUNC_EXIT
} /* CliInit */

/* Find the most recent version of the matching filesystem */
static struct FileSysEntry *internalMatchFileSystemResourceHandler(struct FileSysResource *fsr, ULONG DosType)
{
    struct FileSysEntry *fse, *best_fse = NULL;

    ForeachNode(&fsr->fsr_FileSysEntries, fse)
    {
        if (fse->fse_DosType == DosType)
        {
            if (fse->fse_PatchFlags & (FSEF_HANDLER | FSEF_SEGLIST | FSEF_TASK))
            {
                if (best_fse == NULL || fse->fse_Version > best_fse->fse_Version)
                {
                    best_fse = fse;
                }
            }
        }
    }

    return best_fse;
}

/* See if the BootNode's DeviceNode needs to be patched by
 * an entry in FileSysResource
 *
 * If de->de_DosType == 0, and no dn_SegList nor dn_Handler,
 * then the node uses the 'defseg' handler.
 */
static void internalPatchBootNode(struct FileSysResource *fsr, struct DeviceNode *dn, BPTR defseg)
{
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;
    struct FileSysEntry *fse;

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
    if (de->de_DosType == 0 && dn->dn_Handler == BNULL)
    {
    	D(bug("Dos/CliInit: Neither DosType nor Handler specified, using default filesystem\n"));
        dn->dn_SegList = defseg;
        dn->dn_GlobalVec = (BPTR)-1;
        return;
    }

    /* If no FileSysResource, nothing to do */
    if (fsr == NULL) {
        D(bug("Dos/CliInit: No FileSystem.resource, not patching DeviceNode %p\n", dn));
        return;
    }

    D(bug("Dos/CliInit: Looking for patches for DeviceNode %p\n", dn));

    /*
     * internalMatchFileSystemResourceHandler looks up the filesystem
     */
    fse = internalMatchFileSystemResourceHandler(fsr, de->de_DosType);
    if (fse != NULL)
    {
        D(bug("Dos/CliInit: found 0x%p in FileSystem.resource\n", fse));
        PRINT_DOSTYPE(fse->fse_DosType);

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

static struct MsgPort *mountBootNode(struct DeviceNode *dn, struct FileSysResource *fsr, struct DosLibrary *DOSBase)
{
    struct DosList *dl;

    if ((dn == NULL) || (dn->dn_Name == BNULL))
    	return NULL;

    D(bug("Dos/CliInit: Mounting 0x%p (%b)...\n", dn, dn->dn_Name));

    /* Check if the device is already in DOS list */
    dl = LockDosList(LDF_DEVICES | LDF_READ);
    
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
    	if (dl == (struct DosList *)dn)
    	    break;
    }

    UnLockDosList(LDF_ALL | LDF_READ);

    /* Found in DOS list? Do nothing. */
    if (dl)
    {
    	return dl->dol_Task;
    }

    /* Patch it up, if needed */
    internalPatchBootNode(fsr, dn, DOSBase->dl_Root->rn_FileHandlerSegment);

    if (!dn->dn_Handler && !dn->dn_SegList)
    {
    	/* Don't know how to mount? Error... */
    	return NULL;
    }

    if (AddDosEntry((struct DosList *)dn) != DOSFALSE)
    {
        /*
         * Do not check for ANDF_STARTPROC because:
         * a) On the Amiga ADNF_STARTPROC was not present in KS 1.3 and earlier, there was no deferred mount.
	 * b) In fact if we have something in ExpansionBase, we for sure want it to be mounted.
         */
        D(bug("Dos/CliInit: Added to DOS list, starting up handler... "));

        if (RunHandler(dn, NULL, DOSBase))
        {
            D(bug("dn->dn_Task = 0x%p\n", dn->dn_Task));
            return dn->dn_Task;
        }

        D(bug("Failed\n"));
        RemDosEntry((struct DosList *)dn);
    }

    /*
     * TODO: AddDosEntry() can fail in case of duplicate name. In this case it would be useful
     * to append some suffix. AmigaOS IIRC did the same.
     * This appears to be needed if you have DH0:. DH1:, etc, on your hard drive, and want to
     * connect a friend's hard drive which also has partitions with these names.
     */

    return NULL;
}

static BPTR internalBootLock(struct DosLibrary *DOSBase, struct ExpansionBase *ExpansionBase, struct FileSysResource *fsr)
{
    struct BootNode *bn;
    struct DeviceNode *dn;
    struct MsgPort *mp;
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
    bn = (struct BootNode *)GetHead(&ExpansionBase->MountList);
    D(bug("Dos/CliInit: MountList head: 0x%p\n", bn));

    if (bn == NULL)
        return BNULL;

    dn = bn->bn_DeviceNode;
    mp = mountBootNode(dn, fsr, DOSBase);
    if (!mp)
        return BNULL;

    D(bug("Dos/CliInit: %b (%d) appears usable\n", dn->dn_Name, bn->bn_Node.ln_Pri));

    /* Try to find a Lock for 'name:' */
    name_len = AROS_BSTR_strlen(dn->dn_Name);
    name = AllocVec(name_len + 2, MEMF_ANY);
    if (name != NULL)
    {
	SIPTR err = 0;

        /* Make the volume name a volume: name */
        CopyMem(AROS_BSTR_ADDR(dn->dn_Name), name, name_len);
        name[name_len+0] = ':';
        name[name_len+1] = 0;
        D(bug("Dos/CliInit:   Attempt to Lock(\"%s\")... ", name));

        lock = Lock(name, SHARED_LOCK);
        D(bug("=> 0x%p\n", BADDR(lock)));

        if (lock != BNULL)
        {
            /* If we have a lock, check the per-platform conditional boot code. */
            if (!__dos_IsBootable(DOSBase, lock))
	    {
               	UnLock(lock);
            	lock = BNULL;
            	err = ERROR_OBJECT_WRONG_TYPE; /* Something to more or less reflect "This disk is not bootable" */
            }
        }
        else
        {
            err = IoErr();
        }

        if (!lock)
        {
	    SIPTR dead;

            /* Darn. Not bootable. Try to unmount it. */
            D(bug("Dos/CliInit:   Does not have a bootable filesystem, unmounting...\n"));

            /* It's acceptable if this fails */
            dead = DoPkt(mp, ACTION_DIE, 0, 0, 0, 0, 0);
            D(bug("Dos/CliInit:  ACTION_DIE returned %ld\n", dead));

	    if (dead)
	    {
	    	/*
	    	 * Handlers usually won't remove their DeviceNoces themselves.
	    	 * And even if they do (ACTION_DIE is poorly documented), RemDosEntry()
	    	 * on an already removed entry is safe due to nature of DOS list.
	    	 * What is really prohibited, it's unloading own seglist. Well, resident
	    	 * handlers will never do it, they know...
	    	 */
	    	RemDosEntry((struct DosList *)dn);
	    	dn->dn_Task = NULL;
	    }
            /* DoPkt() clobbered IoErr() */
            SetIoErr(err);
        }

        FreeVec(name);
    }

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


/* 
 * This is what actually gets the Lock() for SYS:,
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
    IPTR BootFlags;
    UBYTE Flags;
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

    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (!ExpansionBase)
	return ERROR_INVALID_RESIDENT_LIBRARY;

    /* It's perfectly fine if this fails. */
    fsr = OpenResource("FileSystem.resource");

    /* Find and Lock the proposed boot device */
    lock = internalBootLock(DOSBase, ExpansionBase, fsr);
    D(bug("Dos/CliInit: Proposed SYS: lock is: %p\n", BADDR(lock)));

    if (lock == BNULL)
    {
    	/*
    	 * We've failed. Inform our parent and exit.
    	 * Immediately after we reply the packet, the parent (Boot Task) can expunge DOSBase.
    	 * This is why we first cleanup, then use internal_ReplyPkt (DOSBase is considered
    	 * invalid).
    	 * Alternatively we could Forbid() before ReplyPkt(), but... Forbid() is so unpolite...
    	 */
    	IPTR err = IoErr();

	CloseLibrary(&ExpansionBase->LibNode);
	CloseLibrary(&DOSBase->dl_lib);

	/* Immediately after ReplyPkt() DOSBase can be freed. So Forbid() until we really quit. */
	internal_ReplyPkt(dp, mp, DOSFALSE, err);
        return err;
    }

    /* Ok, at this point we've succeeded. Inform our parent. */
    ReplyPkt(dp, DOSTRUE, 0);

    /* We're now at the point of no return. */
    DOSBase->dl_Root->rn_BootProc = ((struct FileLock*)BADDR(lock))->fl_Task;
    SetFileSysTask(DOSBase->dl_Root->rn_BootProc);

    AssignLock("SYS", lock);
    lock = Lock("SYS:", SHARED_LOCK);
    if (lock == BNULL)
    {
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
    /* 
     * This early assignment prevents Poseidon from asking for ENV:
     * when popup GUI process is initialized and opens muimaster.library.
     * On m68k this harms, Workbench 1.x disks fail to boot correctly with
     * "Can't cancel ENV:" warning.
     * FIXME: Fix muimaster.library at last, and forget this hack.
     */
    AssignLate("ENV", "SYS:Prefs/Env-Archive");
#endif
    AssignLate("ENVARC", "SYS:Prefs/Env-Archive");

    /*
     * At this point we have only SYS:, nothing more. Mount the rest.
     * We do it after assigning SYS: because in some cases we can have
     * BootNodes with handler name but no seglist (Poseidon could add them for example).
     * This means the handler needs to be loaded from disk (fat-handler for example).
     * Here we can already do it.
     */
    D(bug("Dos/CliInit: Assigns done, mount remaining handlers...\n"));

    BootFlags = ExpansionBase->eb_BootFlags;
    Flags = ExpansionBase->Flags;
    D(bug("Dos/CliInit: BootFlags 0x%x Flags 0x%x\n", BootFlags, Flags));

    ForeachNodeSafe(&ExpansionBase->MountList, bn, tmpbn)
    {
        /*
         * Don't check for return code. Failed is failed.
         * One of failure reasons can be missing handler specification for some DOSType.
         * In this case the DeviceNode will not be mounted, but it will stay in
         * ExpansionBase->MountList. It can be picked up by disk-based program which would
         * read mappings for disk-based handlers from file.
         * This way we could automount e. g. FAT, NTFS, EXT3/2, whatever else, partitions.
         */
	mountBootNode(bn->bn_DeviceNode, fsr, DOSBase);
    }

    CloseLibrary((APTR)ExpansionBase);

    /* Init all the RTF_AFTERDOS code, since we now have SYS:, the dos devices, and all the other assigns */
    D(bug("Dos/CliInit: Calling InitCode(RTF_AFTERDOS, 0)\n"));
    InitCode(RTF_AFTERDOS, 0);

    /* Call the platform-overridable portions */
    D(bug("Dos/CliInit: Calling __dos_Boot(%p, 0x%x, 0x%x)\n", DOSBase, BootFlags, Flags));
    __dos_Boot(DOSBase, BootFlags, Flags);

    D(bug("Dos/CliInit: Boot sequence exited\n"));
    CloseLibrary((APTR)DOSBase);

    /* And exit... */
    return RETURN_OK;
}
