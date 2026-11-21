/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A process made with CreateNewProc() from the seglist of a 68k hunk file
    must not run that code at the native entry point. Where m68kemu.library
    opens, DosEntry() runs the hunk through it; where it does not, the
    process has to end with RETURN_FAIL. This test writes a hunk file whose
    code is "moveq #42,d0; rts" to RAM:, makes a process from its seglist and
    waits for the result that DosEntry() passes to NP_ExitCode: 20 where the
    emulator library does not open, 42 where it ran the code. The child gets
    a CLI and this program's output, so that a library that cannot open says
    so there and not in a requester. No C library is used.
*/
#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define WAIT_TICKS (50 * 60)
#define HUNKFILE   "RAM:dosentry68k.hunk"

/* HUNK_HEADER with one hunk of one longword, HUNK_CODE "moveq #42,d0; rts", HUNK_END */
static const UBYTE hunkfile[] =
{
    0x00, 0x00, 0x03, 0xf3,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x01,  0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x01,  0x00, 0x00, 0x03, 0xe9,  0x00, 0x00, 0x00, 0x01,
    0x70, 0x2a, 0x4e, 0x75,  0x00, 0x00, 0x03, 0xf2
};

static ULONG checks;
static ULONG failures;

/* Written by the child, read by this program. */
struct ExitData
{
    volatile LONG called;
    volatile LONG result;
};

static void check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        bug("DOSENTRY68K FAIL: %s\n", what);
        failures++;
    }
    Printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    Flush(Output());
}

/* Called by DosEntry() in the child, with the result of its entry. */
static void child_exit(IPTR result, IPTR data)
{
    struct ExitData *ed = (struct ExitData *)data;

    ed->result = (LONG)result;
    ed->called = 1;
}

int main(void)
{
    struct ExitData ed = { 0, -1 };
    IPTR hunkinfo = 0;
    struct TagItem htags[] = { { GSLI_68KHUNK, (IPTR)&hunkinfo }, { TAG_DONE, 0 } };
    BOOL in_use = FALSE;
    LONG written = -1;
    BPTR seg, fh;

    fh = Open(HUNKFILE, MODE_NEWFILE);
    if (fh)
    {
        written = Write(fh, (APTR)hunkfile, sizeof(hunkfile));
        Close(fh);
    }
    check(written == (LONG)sizeof(hunkfile), "the hunk file is written to RAM:");

    seg = LoadSeg(HUNKFILE);
    check(seg != BNULL, "LoadSeg() loads the 68k hunk file");
    if (seg == BNULL)
    {
        DeleteFile(HUNKFILE);
        Printf("dosentry68k: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
        return RETURN_FAIL;
    }
    check(GetSegListInfo(seg, htags) && hunkinfo != 0, "GetSegListInfo() says the seglist is a 68k hunk");

    if (hunkinfo != 0)
    {
        struct Process *child;

        Printf("starting the child\n");
        Flush(Output());
        bug("DOSENTRY68K: starting the child\n");
        child = CreateNewProcTags(NP_Seglist, (IPTR)seg,
                                  NP_FreeSeglist, FALSE,
                                  NP_Name, (IPTR)"dosentry68k child",
                                  NP_StackSize, 65536,
                                  NP_Cli, TRUE,
                                  NP_Input, (IPTR)BNULL,
                                  NP_Output, (IPTR)Output(),
                                  NP_CloseOutput, FALSE,
                                  NP_ExitCode, (IPTR)child_exit,
                                  NP_ExitData, (IPTR)&ed,
                                  TAG_DONE);
        check(child != NULL, "CreateNewProc() makes a process from the seglist");
        if (child)
        {
            LONG ticks;

            in_use = TRUE;
            for (ticks = 0; ticks < WAIT_TICKS && !ed.called; ticks += 5)
                Delay(5);
            bug("DOSENTRY68K: child exit called %d result %d\n", (int)ed.called, (int)ed.result);
            check(ed.called != 0, "the child ends within 60 seconds");
            if (ed.called)
            {
                Printf("child result %ld\n", (LONG)ed.result);
                check(ed.result == RETURN_FAIL || ed.result == 42,
                      "the child's result is RETURN_FAIL (no emulator) or 42 (the emulator ran the code)");
                /* Let the child leave DosEntry() before its seglist goes. */
                Delay(25);
                in_use = FALSE;
            }
        }
    }

    /* The seglist stays loaded if the child did not end: it may still be running in it. */
    if (!in_use)
        UnLoadSeg(seg);
    DeleteFile(HUNKFILE);

    Printf("dosentry68k: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
    Flush(Output());
    return failures ? RETURN_FAIL : RETURN_OK;
}
