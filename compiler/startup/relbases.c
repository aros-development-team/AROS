/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Per-task relative-base offset table for programs that link the
          per-task (relbase) C runtime linklibs.

    posixc.library (and its rel-libraries stdc/stdcio) is a "pertaskbase"
    library: every Task must reach it through its own per-task base.  The
    default absolute linklib exposes a single, process-wide base pointer that
    the C startup sets from the main Task.  That is fine for single-threaded
    programs, but a pthread worker never runs the C startup, so it reads that
    pointer as NULL and bus-faults on its first C runtime call.

    Linking a program against the relbase linklibs (posixc_rel, stdc_rel,
    stdcio_rel) instead routes every call through the *_relwrapper stubs, which
    fetch the base from __aros_getoffsettable() + __aros_rellib_offset_<Base>.
    This file supplies both the offsets and __aros_getoffsettable(), returning a
    small per-Task table of base pointers that is populated lazily on each
    Task's first call.  Because a worker's posixc base is created (through
    OpenLibrary below) with the pertaskbase parent fallback, it inherits the
    creator Process' state and routes its descriptors there, while keeping its
    own errno/stdio - i.e. it is thread-safe.
*/

#include <proto/exec.h>
#include <proto/task.h>
#include <proto/posixc.h>
#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <aros/debug.h>
#include <libraries/stdc.h>

#include <setjmp.h>
#include <stddef.h>

struct __rel_bases
{
    struct PosixCBase  *PosixCBase;
    struct StdCBase    *StdCBase;
    struct StdCIOBase  *StdCIOBase;
};

/* Byte offset of each base pointer within the table, referenced by the
   *_relwrapper stubs in libposixc_rel.a / libstdc_rel.a / libstdcio_rel.a. */
const IPTR __aros_rellib_offset_PosixCBase = offsetof(struct __rel_bases, PosixCBase);
const IPTR __aros_rellib_offset_StdCBase   = offsetof(struct __rel_bases, StdCBase);
const IPTR __aros_rellib_offset_StdCIOBase = offsetof(struct __rel_bases, StdCIOBase);

/* Base for the per-task storage calls below (task.resource); the *_relwrapper
   stubs and __GM_* helpers reach it through this global. */
APTR TaskResBase;

static LONG __rel_slot;

static LONG __rel_ensure_slot(void)
{
    if (!__rel_slot)
    {
        Forbid();
        if (!TaskResBase)
            TaskResBase = OpenResource((CONST_STRPTR)"task.resource");
        if (!__rel_slot && TaskResBase)
            __rel_slot = AllocTaskStorageSlot();
        Permit();
    }
    return __rel_slot;
}

char *__aros_getoffsettable(void)
{
    struct __rel_bases *t;

    if (!__rel_ensure_slot())
        return NULL;

    t = (struct __rel_bases *)GetTaskStorageSlot(__rel_slot);
    if (t == NULL)
    {
        t = AllocMem(sizeof(*t), MEMF_PUBLIC | MEMF_CLEAR);
        if (t == NULL)
            return NULL;

        /* Publish before populating: OpenLibrary() never re-enters this table
           (it is a pure exec call) and per-task storage is private to this
           Task, so no other context can observe the half-filled table. */
        SetTaskStorageSlot(__rel_slot, (IPTR)t);

        t->PosixCBase = (struct PosixCBase *)OpenLibrary((CONST_STRPTR)"posixc.library", 0);
        if (t->PosixCBase)
        {
            /* Adopt posixc's OWN per-task stdc/stdcio bases (published in the
               public fields by posixc's OpenLib hook).  Do NOT open our own -
               stdc/stdcio create a fresh base on every OpenLibrary() and would
               diverge from the base posixc uses internally, breaking shared
               errno and the program exit jmp_buf. */
            t->StdCBase   = t->PosixCBase->StdCBase;
            t->StdCIOBase = t->PosixCBase->StdCIOBase;
        }
    }

    return (char *)t;
}

/* Reserve the per-task slot as early as possible, before any worker exists. */
static int __rel_init(void)
{
    __rel_ensure_slot();
    return TRUE;
}

ADD2INIT(__rel_init, -60);

/*
 * stdc's program startup entry (stdc_startup.c) lives in the absolute stdc
 * linklib, which -nostdc drops; the rel linklib does not provide it.  Without
 * it __stdc_program_startup() is never called, so the program's stdc
 * startup_errorptr/exit_jmpbuf stay uninitialised and exit()/_exit() fault
 * ("exit without proper initialization").  Provide the same PROGRAM_ENTRIES
 * entry here so a rel-linked program initialises its stdc exit machinery
 * (through its own per-task StdCBase) before main() runs.
 */
static void __relbases_stdc_startup(struct ExecBase *SysBase)
{
    jmp_buf exitjmp;

    if (setjmp(exitjmp) == 0)
    {
        __stdc_program_startup(exitjmp, (int *)&__startup_error);
        __startup_entries_next();
    }

    __stdc_program_end();
}

ADD2SET(__relbases_stdc_startup, PROGRAM_ENTRIES, 0);
