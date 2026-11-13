/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Regression test: a program that uses getpwuid() must not leave its
    posixc.library and usergroup.library bases behind when it exits.

    posixc opens usergroup.library on the first getpw*() call, and usergroup
    holds a reference on the same per-task posixc base, so neither base used
    to reach its final close: every such program left both bases open, and a
    later program in the same process could be handed the stale posixc base.

    This program does not use posixc itself. It runs usergroup_release_child
    in its own process with RunCommand(), as a Shell does, and compares the
    open counts of usergroup.library and posixc.library before and after.
    usergroup.library is opened here first, so that the child does not load
    it: the first load of a library is a separate case.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/execbase.h>

#define RUNS 3

static LONG opencount(const char *name)
{
    struct Library *lib;
    LONG count = -1;

    Forbid();
    lib = (struct Library *)FindName(&SysBase->LibList, (CONST_STRPTR)name);
    if (lib)
        count = lib->lib_OpenCnt;
    Permit();

    return count;
}

int main(void)
{
    struct Library *ug;
    BPTR seg;
    LONG ug0, px0, ug1, px1, rc = 0;
    int i;

    ug = OpenLibrary((CONST_STRPTR)"usergroup.library", 0);
    seg = LoadSeg((CONST_STRPTR)"PROGDIR:usergroup_release_child");
    if (!ug || !seg)
    {
        Printf((CONST_STRPTR)"USERGROUP RELEASE FAIL: setup usergroup=%s child=%s\n",
               ug ? "ok" : "missing", seg ? "ok" : "missing");
        if (seg) UnLoadSeg(seg);
        if (ug) CloseLibrary(ug);
        return RETURN_FAIL;
    }

    ug0 = opencount("usergroup.library");
    px0 = opencount("posixc.library");
    for (i = 0; i < RUNS && rc == 0; i++)
        rc = RunCommand(seg, 65536, (STRPTR)"\n", 1);
    ug1 = opencount("usergroup.library");
    px1 = opencount("posixc.library");

    Printf((CONST_STRPTR)"USERGROUP RELEASE %s: runs=%ld child_rc=%ld usergroup %ld->%ld posixc %ld->%ld\n",
           (rc == 0 && ug1 == ug0 && px1 == px0) ? "PASS" : "FAIL",
           (LONG)i, rc, ug0, ug1, px0, px1);

    UnLoadSeg(seg);
    CloseLibrary(ug);
    return (rc == 0 && ug1 == ug0 && px1 == px0) ? RETURN_OK : RETURN_FAIL;
}
