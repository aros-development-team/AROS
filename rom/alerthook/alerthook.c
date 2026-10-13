/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Alert.Hook for AROS
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/config.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

/*
 * The alert texts come from exec.library (rom/exec/alertstrings.c). This hook
 * is only part of the amiga-m68k ROM, linked into the same image as exec, so
 * it uses exec's tables rather than carrying a second copy of them.
 */
extern STRPTR Alert_GetString(ULONG alertnum, STRPTR buf);

static const UBYTE Alerthook_name[];
static const UBYTE Alerthook_version[];
static AROS_UFP3(ULONG, Alerthook_init,
    AROS_UFPA(void *,               dummy1,  D0),
    AROS_UFPA(BPTR,                 dummy2,  A0),
    AROS_UFPA(struct ExecBase *,    SysBase, A6)
);


extern void Alerthook_end(void);

const struct Resident Alerthook_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Alerthook_resident,
    (APTR)&Alerthook_end,
    RTF_COLDSTART,
    41,
    NT_UNKNOWN,
    -45,
    (UBYTE *)Alerthook_name,
    (UBYTE *)&Alerthook_version[6],
    (APTR)&Alerthook_init,
};

static const UBYTE Alerthook_name[] = "alert.hook\r\n";
static const UBYTE Alerthook_version[] = "$VER: alert.hook 41.8 " ADATE;
static UBYTE *const nomem = "\x38\x0f" "Not Enough Memory! ";
static UBYTE *const sfail = "\x38\x0f" "Software Failure! ";
static UBYTE *const recov = "\x38\x0f" "Recoverable Alert! ";
static UBYTE *const mouse = "\x01\x50\x0f" "Press mouse button to continue.";
static UBYTE *const fmtstring = "\xa8\x2a" "Task:   %08lx - ";
static UBYTE *const errstring = "\x1e" "Error  %04x %04x - ";
static UBYTE *const tasknotfound = "--task not found--";

/* This is the callback for RawDoFmt() */
AROS_UFH2(void, putChProc,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, buf, A3))
{
    AROS_LIBFUNC_INIT

    **buf = chr;
    *buf += 1;

    AROS_LIBFUNC_EXIT
}

/* This function copies a string, but also returns a pointer to the
   '\0' at the end of the string. This way I can concat the strings
   which have '\0' in them for other reasons.

   Note len == -1 is equivalent to strcpy(dest,src).
*/
static STRPTR
mystrcpy(STRPTR dest, STRPTR src, LONG len)
{
    while(len && *src)
    {
        *dest++ = *src++;
        len--;
    }
    *dest++ = 0;
    return dest;
}

static AROS_UFH3(ULONG, Alerthook_init,
    AROS_UFHA(void *,               dummy1,  D0),
    AROS_UFHA(BPTR,                 dummy2,  A0),
    AROS_UFHA(struct ExecBase *,    SysBase, A6)
)
{
    AROS_LIBFUNC_INIT

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
        Clear memory location zero (a cookie for the delayed guru after reset).
        Some machines will keep putting a guru up during every reset if I don't
        do this here.
    */
    ULONG *location_zero = (void *)0;

    *location_zero = 0;
#endif

    D(bug("AlertHook: *******************\n"));
    if(SysBase->LastAlert[0] != -1)
    {
        struct IntuitionBase *IntuitionBase;
        struct Task *task;
        UBYTE buffer[256], *buf, *tname;

        D(bug("alert.hook: LastAlert[0] = 0x%lx\n", SysBase->LastAlert[0]));

        buffer[0] = 0;
        buf = &buffer[1];

        if(SysBase->LastAlert[0] & AG_NoMemory)
            buf = mystrcpy(buf, nomem, -1);
        else if(SysBase->LastAlert[0] & AT_DeadEnd)
            buf = mystrcpy(buf, sfail, -1);
        else
            buf = mystrcpy(buf, recov, -1);

        *buf++ = 1;

        buf = mystrcpy(buf, mouse, -1);
        *buf++ = 1; *buf++ = 0;

        /* Find out the task name. The node type must be correct. */
        task = (struct Task *)SysBase->LastAlert[1];
        if( (
                  (task->tc_Node.ln_Type == NT_TASK)
               || (task->tc_Node.ln_Type == NT_PROCESS)
            )
            && (task->tc_Node.ln_Name != NULL)
          )
            tname = task->tc_Node.ln_Name;
        else
            tname = tasknotfound;

        RawDoFmt(fmtstring, &SysBase->LastAlert[1], (void *)putChProc, &buf);
        buf = mystrcpy(buf - 1, tname, 30);
        *buf++ = 1; *buf++ = 0;

        /* Use this variable to hold the current address */
        tname = buf++;
        RawDoFmt(errstring, &SysBase->LastAlert[0], (void *)putChProc, &buf);

        /* Alert_GetString() returns the terminator; step past it */
        buf = Alert_GetString(SysBase->LastAlert[0], &buf[-1]) + 1;
        *buf++ = 0;

        /* This rather strange contraption will centre the string. */
        *((UBYTE *)tname) = (82 - (buf - tname)) << 2;

        IntuitionBase = (struct IntuitionBase *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION);

        if(IntuitionBase)
        {
            if(IntuitionBase->LibNode.lib_Version >= 39)
                TimedDisplayAlert(SysBase->LastAlert[0] & AT_DeadEnd, buffer, 0x38, 500);
            else
                DisplayAlert(SysBase->LastAlert[0] & AT_DeadEnd, buffer, 0x38);

            CloseLibrary((struct Library *)IntuitionBase);
        } else {
            D(bug("alert.hook: No intuition, dumping message\n"));
            D(bug("alert.hook: %s\n", buffer));
        }

        SysBase->LastAlert[0] = -1;
    }
#if 0
    SysBase->LastAlert[0] = old;
    SysBase->LastAlert[1] = NULL;
#endif
    return FALSE;

    AROS_LIBFUNC_EXIT
}
