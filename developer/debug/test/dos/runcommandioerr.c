/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    After RunCommand() IoErr() is the secondary result of the command that
    ran. RunCommand() puts the command's arguments into the buffer of
    Input() and flushes that stream when the command has returned. A command
    that did not read them leaves them there, the flush seeks back over
    them, and on a stream that cannot seek the seek fails with an error of
    its own.

    This program loads itself and runs itself with RunCommand(). Started
    with the argument "child" it leaves a known error and a known result
    without reading Input(). The parent does that twice: with the Input() it
    was started with, and with NIL:. For each it says whether the stream can
    seek, since only one that cannot shows the defect. No C library is used.
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define CHILD_ERROR  ERROR_OBJECT_NOT_FOUND
#define CHILD_RESULT RETURN_WARN
#define STACK        65536

struct DosLibrary *DOSBase;

static ULONG checks;
static ULONG failures;

static void check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
        failures++;
    Printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    Flush(Output());
}

/* Run this program as "child" with RunCommand() on the current Input() */
static void run(BPTR seg, CONST_STRPTR stream)
{
    LONG rc, err;
    BOOL seeks;

    SetIoErr(0);
    seeks = Seek(Input(), 0, OFFSET_CURRENT) != -1;
    Printf("     %s: %s\n", stream, seeks ? "can seek" : "cannot seek");

    SetIoErr(0);
    rc = RunCommand(seg, STACK, "child\n", 6);
    err = IoErr();

    Printf("     %s: result %ld, IoErr %ld\n", stream, rc, err);
    check(rc == CHILD_RESULT, "RunCommand() returns the result of the command");
    check(err == CHILD_ERROR, "IoErr() after RunCommand() is the command's");
}

__startup static AROS_PROCH(Start, argstr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    char name[256];
    BPTR seg, nil, old;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (!DOSBase)
        return RETURN_FAIL;

    /* run by the parent below: an error and a result, and Input() untouched */
    if (argsize >= 5 && argstr[0] == 'c' && argstr[1] == 'h' && argstr[2] == 'i' &&
        argstr[3] == 'l' && argstr[4] == 'd')
    {
        SetIoErr(CHILD_ERROR);
        CloseLibrary(&DOSBase->dl_lib);
        return CHILD_RESULT;
    }

    seg = BNULL;
    if (GetProgramName(name, sizeof(name)))
        seg = LoadSeg(name);
    if (!seg)
    {
        Printf("FAIL the program could not load itself (%s)\n", name);
        CloseLibrary(&DOSBase->dl_lib);
        return RETURN_FAIL;
    }

    run(seg, "Input()");

    nil = Open("NIL:", MODE_OLDFILE);
    if (nil)
    {
        old = SelectInput(nil);
        run(seg, "NIL:");
        SelectInput(old);
        Close(nil);
    }
    else
        Printf("SKIP NIL: does not open\n");

    UnLoadSeg(seg);
    Printf("runcommandioerr: %lu checks, %lu failed\n", checks, failures);
    CloseLibrary(&DOSBase->dl_lib);
    return failures ? RETURN_FAIL : RETURN_OK;

    AROS_PROCFUNC_EXIT
}
