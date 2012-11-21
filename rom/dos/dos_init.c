/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header for dos.library
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/partition.h>
#include <utility/tagitem.h>
#include <resources/filesysres.h>

#include LC_LIBDEFS_FILE

#include "dos_intern.h"

static const UBYTE version[];
extern const char LIBEND;

AROS_UFP3S(struct DosLibrary *, DosInit,
    AROS_UFPA(ULONG, dummy, D0),
    AROS_UFPA(BPTR, segList, A0),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

const struct Resident Dos_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Dos_resident,
    (APTR)&LIBEND,
    0,                  /* We don't autoinit */
    VERSION_NUMBER,
    NT_LIBRARY,
    RESIDENTPRI,
    MOD_NAME_STRING,
    (STRPTR)&version[6],
    DosInit
};

static const UBYTE version[] = VERSION_STRING;

extern const ULONG err_Numbers[];
extern const char err_Strings[];

static void DosExpunge(struct DosLibrary *DOSBase);

extern const APTR GM_UNIQUENAME(FuncTable)[];

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(INITLIB)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXPUNGELIB)
DEFINESET(INIT)
DEFINESET(EXIT)
DEFINESET(INITLIB)
DEFINESET(EXPUNGELIB)

static void init_fs(struct DosLibrary *DOSBase)
{
    struct FileSysResource *fsr;
    struct Library *PartitionBase;

    PartitionBase = OpenLibrary("partition.library", 3);
    if (PartitionBase) {
        LoadBootFileSystems();
        CloseLibrary(PartitionBase);
    }

        /*
         * Set dl_Root->rn_FileHandlerSegment to the AFS handler,
         * if it's been loaded. Otherwise, use the first handler
         * on the FileSystemResource list that has fse_PatchFlags
         * set to mark it with a valid SegList
         */
        if ((fsr = OpenResource("FileSystem.resource")))
        {
        struct FileSysEntry *fse;
            BPTR defseg = BNULL;
            const ULONG DosMagic = 0x444f5301; /* DOS\001 */

            ForeachNode(&fsr->fsr_FileSysEntries, fse)
            {
                if ((fse->fse_PatchFlags & FSEF_SEGLIST) && fse->fse_SegList)
                {
                    /* We prefer DOS\001 */
                if (fse->fse_DosType == DosMagic)
                    {
                        defseg = fse->fse_SegList;
                        break;
                    }
                /* This will remember the first defined seglist */
                    if (!defseg)
                        defseg = fse->fse_SegList;
                }
            }
        DOSBase->dl_Root->rn_FileHandlerSegment = defseg;
        /* Add all that have both Handler and SegList defined to the Resident list */
        ForeachNode(&fsr->fsr_FileSysEntries, fse)
        {
                if ((fse->fse_PatchFlags & FSEF_HANDLER) &&
                    (fse->fse_PatchFlags & FSEF_SEGLIST) &&
                    (fse->fse_Handler != BNULL) &&
                    (fse->fse_SegList != BNULL))
                {
                    D(bug("[DosInit] Adding \"%b\" (%p) at %p to the resident list\n",
                        fse->fse_Handler, BADDR(fse->fse_Handler), BADDR(fse->fse_SegList)));
                    AddSegment(AROS_BSTR_ADDR(fse->fse_Handler), fse->fse_SegList, CMD_SYSTEM);
            }
        }
    }
}

/*
 * Init routine is intentionally written by hands in order to be reentrant.
 * Reentrancy is needed when there are already some devices mounted, but
 * all of them are not bootable. And now we insert a floppy with OS3.1
 * bootblock. It reenters this function...
 */

AROS_UFH3S(struct DosLibrary *, DosInit,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase;

    if (!SysBase)
        SysBase = sysBase;

    if (!set_call_funcs(SETNAME(INIT), 1, 1))
        return NULL;
    
    DOSBase = (struct DosLibrary *)FindName(&SysBase->LibList, "dos.library");

    D(bug("[DosInit] DOSBase 0x%p\n", DOSBase));

    if (!DOSBase)
    {
        IPTR *taskarray;
        struct DosInfo *dosinfo;

        D(bug("[DosInit] Creating dos.library...\n"));

        DOSBase = (struct DosLibrary *)MakeLibrary(GM_UNIQUENAME(FuncTable), NULL, NULL, sizeof(struct IntDosBase), BNULL);
        if (!DOSBase)
            return NULL;

        /* Initialize our header */
        DOSBase->dl_lib.lib_Node.ln_Name = MOD_NAME_STRING;
        DOSBase->dl_lib.lib_Node.ln_Type = NT_LIBRARY;
        DOSBase->dl_lib.lib_Node.ln_Pri  = RESIDENTPRI;
        DOSBase->dl_lib.lib_Version      = VERSION_NUMBER;
        DOSBase->dl_lib.lib_Revision     = REVISION_NUMBER;
        DOSBase->dl_lib.lib_IdString     = (char *)&version[6];
        DOSBase->dl_lib.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;

        /*
         * These two are allocated together with DOSBase, for reduced fragmentation.
         * Structure pointed to by dl_Errors is intentionally read-write - who knows...
         */
        DOSBase->dl_Root   = &((struct IntDosBase *)DOSBase)->rootNode;
        DOSBase->dl_Errors = &((struct IntDosBase *)DOSBase)->errors;

        DOSBase->dl_Errors->estr_Nums    = (LONG *)err_Numbers;
        DOSBase->dl_Errors->estr_Strings = (STRPTR)err_Strings;

        /* Init the RootNode structure */
        dosinfo = AllocMem(sizeof(struct DosInfo), MEMF_PUBLIC|MEMF_CLEAR);
        if (!dosinfo)
        {
            DosExpunge(DOSBase);
            return NULL;
        }

        DOSBase->dl_Root->rn_Info = MKBADDR(dosinfo);

        taskarray = AllocMem(sizeof(IPTR) + sizeof(APTR) * 20, MEMF_CLEAR);
        if (!taskarray)
        {
            DosExpunge(DOSBase);
            return NULL;
        }

        taskarray[0] = 20;
        DOSBase->dl_Root->rn_TaskArray = MKBADDR(taskarray);

        NEWLIST((struct List *)&DOSBase->dl_Root->rn_CliList);
        InitSemaphore(&DOSBase->dl_Root->rn_RootLock);

        InitSemaphore(&dosinfo->di_DevLock);
        InitSemaphore(&dosinfo->di_EntryLock);
        InitSemaphore(&dosinfo->di_DeleteLock);

        /* Initialize for Stricmp */
        DOSBase->dl_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
        if (!DOSBase->dl_UtilityBase)
        {
            DosExpunge(DOSBase);
            return NULL;
        }

        /* Initialize for the fools that illegally used this field */
        DOSBase->dl_IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);

        /*
         * iaint:
         * I know this is bad, but I also know that the timer.device
         * will never go away during the life of dos.library. I also
         * don't intend to make any I/O calls using this.
         *
         * I also know that timer.device does exist in the device list
         * at this point in time.
         *
         * I can't allocate a timerequest/MsgPort pair here anyway,
         * because I need a separate one for each caller to Delay().
         * However, CreateIORequest() will fail if MsgPort == NULL, so we
         * supply some dummy value.
         */
        DOSBase->dl_TimeReq = CreateIORequest((APTR)0xC0DEBAD0, sizeof(struct timerequest));
        if (!DOSBase->dl_TimeReq)
        {
            DosExpunge(DOSBase);
            return NULL;
        }

        if (OpenDevice("timer.device", UNIT_VBLANK, &DOSBase->dl_TimeReq->tr_node, 0))
        {
            DeleteIORequest(DOSBase->dl_TimeReq);
            DOSBase->dl_TimeReq = NULL;
            DosExpunge(DOSBase);
            return NULL;
        }

        /* Call platform-specific init code (if any) */
        if (!set_call_libfuncs(SETNAME(INITLIB), 1, 1, DOSBase))
        {
            DosExpunge(DOSBase);
            return NULL;
        }

        /* debug.library is optional, so don't check result */
        DebugBase = OpenLibrary("debug.library", 0);

        /* Initialization finished */
        AddLibrary(&DOSBase->dl_lib);

    init_fs(DOSBase);
   }

   /* Try to boot */
   if (CliInit(NULL) == RETURN_OK)
   {
        /*
         * We now restart the multitasking - this is done
         * automatically by RemTask() when it switches.
         */
        RemTask(NULL);

        /* We really really shouldn't ever get to this line. */
    }

    DosExpunge(DOSBase);
    return NULL;
    
    AROS_USERFUNC_EXIT
}

static void DosExpunge(struct DosLibrary *DOSBase)
{
    struct DosInfo *dinfo = BADDR(DOSBase->dl_Root->rn_Info);
    struct Segment *seg, *stmp;

    D(bug("[DosInit] Expunge...\n"));

    /* If we have anything in the Dos List,
     * we can't die.
     */
    if (dinfo->di_DevInfo != BNULL)
    {
#if DEBUG
        struct DosList *dol;

        bug("[DosInit] Entries still in the Dos List, can't expunge.\n");
        for (dol = BADDR(dinfo->di_DevInfo); dol != NULL; dol = BADDR(dol->dol_Next))
        {
            bug("[DosInit] %d '%b'\n", dol->dol_Type, dol->dol_Name);
        }
#endif
        return;
    }

    if (DOSBase->dl_lib.lib_OpenCnt)
    {
        /*
         * Someone is holding us... Perhaps some handler started subprocess
         * which didn't quit. Who knows...
         */
        D(bug("[DosInit] Open count is %d, can't expunge\n"));
        return;
    }

    /* Call platform-specific expunge code (if any) */
    if (!set_call_libfuncs(SETNAME(EXPUNGELIB), -1, 1, DOSBase))
    {
        D(bug("[DosInit] Platform-dependent code failed to expunge\n"));
        return;
    }

    /* Close some libraries */
    CloseLibrary(DebugBase);
    CloseLibrary((APTR)DOSBase->dl_IntuitionBase);
    CloseLibrary((APTR)DOSBase->dl_UtilityBase);

    /* Free the timer device */
    if (DOSBase->dl_TimeReq)
    {
        CloseDevice(&DOSBase->dl_TimeReq->tr_node);
        DeleteIORequest(DOSBase->dl_TimeReq);
    }

    if (dinfo)
    {
        /* Remove all segments */
        for (seg = BADDR(dinfo->di_ResList); seg != NULL; seg = stmp)
        {
            stmp = BADDR(seg->seg_Next);
            FreeVec(seg);
        }
        FreeMem(dinfo, sizeof(*dinfo));
    }

    /* Free memory */
    FreeMem(BADDR(DOSBase->dl_Root->rn_TaskArray), sizeof(IPTR) + sizeof(APTR) * 20);

    if (DOSBase->dl_lib.lib_Node.ln_Succ)
    {
        /*
         * A fresh DOSBase after creation is filled with NULLs.
         * ln_Succ will be set to something only after AddLibrary().
         */
        Remove(&DOSBase->dl_lib.lib_Node);
    }

    FreeMem((char *)DOSBase - DOSBase->dl_lib.lib_NegSize, DOSBase->dl_lib.lib_NegSize + DOSBase->dl_lib.lib_PosSize);

    set_call_funcs(SETNAME(EXIT), -1, 0);
    
    D(bug("%s: Expunged.\n", __func__));
}

/*
 * Simple open and close routines.
 * We never auto-expunge, because if we ever do this,
 * we won't be able to come up again. BTW, LDDemon constantly holds us open,
 * so we always have at least one user.
 */
AROS_LH1(struct DosLibrary *, OpenLib,
         AROS_LHA(ULONG, version, D0),
         struct DosLibrary *, DOSBase, 1, Dos)
{
    AROS_LIBFUNC_INIT

    /* I have one more opener. */
    DOSBase->dl_lib.lib_OpenCnt++;
    return DOSBase;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, CloseLib,
         struct DosLibrary *, DOSBase, 2, Dos)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    DOSBase->dl_lib.lib_OpenCnt--;
    return BNULL;

    AROS_LIBFUNC_EXIT
}
