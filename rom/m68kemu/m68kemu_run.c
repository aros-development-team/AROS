/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

    m68kemu.library — RunHunk / RunFile entry points
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>

#include "m68kemu_intern.h"
#include "m68kemu_shadow.h"
#include "m68kemu_thunks.h"

/* ── Emulator child process entry point ────────────────────────── */

static LONG emu_proc_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct M68KEmuContext *ctx = (struct M68KEmuContext *)me->pr_Task.tc_UserData;

    if (!ctx) return -1;

    ctx->exit_code = M68KEmu_Execute(ctx);
    return ctx->exit_code;
}

/* ── Common setup + execution for both RunHunk and RunFile ─────── */

static LONG setup_and_execute(struct M68KEmuContext *ctx, ULONG entry,
                              CONST_STRPTR argPtr, ULONG argSize,
                              ULONG stackSize, const char *programName,
                              struct M68KEmuLibBase *M68KEmuBase)
{
    ULONG m68k_sysbase, m68k_argptr;
    LONG result = -1;

    /* Set up fake exec.library base (lib_id=0) */
    m68k_sysbase = M68KEmu_SetupLibBase(ctx, 0, "exec.library",
                                         M68KEMU_MAX_LVO,
                                         m68kemu_thunks_exec,
                                         m68kemu_thunks_exec_count,
                                         m68kemu_thunks_exec_gen,
                                         m68kemu_thunks_exec_gen_count);
    if (!m68k_sysbase)
        return -1;

    /* Set up fake dos.library base (lib_id=1) */
    M68KEmu_SetupLibBase(ctx, 1, "dos.library",
                          M68KEMU_MAX_LVO,
                          m68kemu_thunks_dos,
                          m68kemu_thunks_dos_count,
                          m68kemu_thunks_dos_gen,
                          m68kemu_thunks_dos_gen_count);

    /* Write SysBase pointer at address 4 (AbsExecBase) */
    m68k_write32(ctx, 4, m68k_sysbase);

    /* Copy argument string into containment space */
    m68k_argptr = 0;
    if (argPtr && argSize > 0)
    {
        m68k_argptr = M68KEmu_HeapAlloc(ctx, argSize + 1, 0);
        if (m68k_argptr)
            CopyMem((APTR)argPtr, m68k_to_host(ctx, m68k_argptr), argSize);
    }

    if (programName)
    {
        strncpy(ctx->program_name, programName, 255);
        ctx->program_name[255] = 0;
    }

    /* Init ExecBase shadow data fields */
    shadow_init_execbase(ctx, m68k_sysbase, SysBase);

    /* Auto-open libraries referenced by C startup tables */
    M68KEmu_AutoOpenLibs(ctx);

    /* Store arguments for the entry point —
       M68KEmu_Execute will set registers after reset */
    ctx->entry_point  = entry;
    ctx->m68k_argptr  = m68k_argptr;
    ctx->m68k_argsize = argSize;
    ctx->m68k_sysbase = m68k_sysbase;

    /* If stackSize is 0, run directly (non-threaded) */
    if (stackSize == 0)
        return M68KEmu_Execute(ctx);

    /* Spawn emulator in its own AROS process.
     * NP_Synchronous=TRUE makes us block until the child exits.
     */
    {
        BPTR curdir = CurrentDir(BNULL);
        CurrentDir(curdir);  /* restore */

        BPTR dirlock = DupLock(curdir);

        struct TagItem tags[] = {
            { NP_Entry,       (IPTR)emu_proc_entry },
            { NP_Name,        (IPTR)"m68kemu" },
            { NP_Synchronous, TRUE },
            { NP_UserData,    (IPTR)ctx },
            { NP_FreeSeglist, FALSE },
            { NP_Input,       (IPTR)Input() },
            { NP_CloseInput,  FALSE },
            { NP_Output,      (IPTR)Output() },
            { NP_CloseOutput, FALSE },
            { NP_CurrentDir,  (IPTR)dirlock },
            { TAG_DONE,       0 }
        };

        struct Process *proc = CreateNewProc(tags);
        if (proc)
        {
            result = ctx->exit_code;
        }
        else
        {
            UnLock(dirlock);
            result = -1;
        }
    }

    return result;
}

/*****************************************************************************

    NAME */
#include <proto/m68kemu.h>

        AROS_LH4(LONG, RunHunk,

/*  SYNOPSIS */
        AROS_LHA(BPTR,        segList,   D1),
        AROS_LHA(ULONG,       stackSize, D2),
        AROS_LHA(CONST_STRPTR, argPtr,   D3),
        AROS_LHA(ULONG,       argSize,   D4),

/*  LOCATION */
        struct M68KEmuLibBase *, M68KEmuBase, 5, M68kemu)

/*  FUNCTION
        Execute an m68k AmigaOS hunk binary under emulation.
        All library calls from the m68k code are trapped and
        dispatched to native AROS implementations.

    INPUTS
        segList   - Loaded m68k hunk segment list (from LoadSeg)
        stackSize - Stack size for the emulated process
        argPtr    - Command arguments string
        argSize   - Length of arguments

    RESULT
        Return code from the m68k program, or -1 on failure.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct M68KEmuContext *ctx;
    ULONG entry;
    LONG result = -1;

    ctx = M68KEmu_CreateContext(M68KEmuBase);
    if (!ctx)
        return -1;

    entry = M68KEmu_LoadHunks(ctx, segList);
    if (entry)
        result = setup_and_execute(ctx, entry, argPtr, argSize,
                                   stackSize, NULL, M68KEmuBase);

    M68KEmu_DestroyContext(ctx);
    return result;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    RunFile — load and execute an m68k hunk binary from a file path
*****************************************************************************/
#include <proto/m68kemu.h>

        AROS_LH4(LONG, RunFile,
        AROS_LHA(CONST_STRPTR, fileName, D1),
        AROS_LHA(ULONG,       stackSize, D2),
        AROS_LHA(CONST_STRPTR, argPtr,   D3),
        AROS_LHA(ULONG,       argSize,   D4),
        struct M68KEmuLibBase *, M68KEmuBase, 6, M68kemu)
{
    AROS_LIBFUNC_INIT

    struct M68KEmuContext *ctx;
    BPTR fh;
    LONG fileSize;
    UBYTE *fileData;
    ULONG entry;
    LONG result = -1;

    fh = Open(fileName, MODE_OLDFILE);
    if (!fh) return -1;

    Seek(fh, 0, OFFSET_END);
    fileSize = Seek(fh, 0, OFFSET_BEGINNING);
    if (fileSize <= 0) { Close(fh); return -1; }

    fileData = (UBYTE *)AllocMem(fileSize, MEMF_ANY);
    if (!fileData) { Close(fh); return -1; }

    if (Read(fh, fileData, fileSize) != fileSize)
    { FreeMem(fileData, fileSize); Close(fh); return -1; }
    Close(fh);

    ctx = M68KEmu_CreateContext(M68KEmuBase);
    if (!ctx) { FreeMem(fileData, fileSize); return -1; }

    entry = M68KEmu_LoadHunksFromMemory(ctx, fileData, fileSize);
    FreeMem(fileData, fileSize);

    if (entry)
        result = setup_and_execute(ctx, entry, argPtr, argSize,
                                   stackSize, fileName, M68KEmuBase);

    M68KEmu_DestroyContext(ctx);
    return result;

    AROS_LIBFUNC_EXIT
}
