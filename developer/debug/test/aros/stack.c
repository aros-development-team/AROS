/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test: the start-up code honours a program's __stack minimum.

    Runs stackchild (built beside this test) through RunCommand() with a 16 KB
    stack, on this task, twice each: once returning from main() and once
    calling exit() from deep in a recursion. Checks the return codes, that the
    program's atexit() handler ran in the exit case, and that this task's stack
    bounds are the same afterwards.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <stdio.h>
#include <string.h>

#define CHILD_STACK 16384

static BPTR seg;

static void cleanup(void)
{
    DeleteVar((CONST_STRPTR)"stacktest_atexit", GVF_LOCAL_ONLY);
    if (seg)
        UnLoadSeg(seg);
}

static int run(const char *mode, LONG expected, BOOL want_atexit)
{
    struct Task *me = FindTask(NULL);
    APTR lower = me->tc_SPLower, upper = me->tc_SPUpper;
    char args[64], flag[8];
    LONG rc;

    DeleteVar((CONST_STRPTR)"stacktest_atexit", GVF_LOCAL_ONLY);
    snprintf(args, sizeof(args), "%lx %s\n", (unsigned long)(IPTR)me, mode);
    rc = RunCommand(seg, CHILD_STACK, (STRPTR)args, strlen(args));
    if (rc != expected)
    {
        printf("FAIL %s: return code %ld, expected %ld\n", mode, (long)rc, (long)expected);
        return 0;
    }
    if (me->tc_SPLower != lower || me->tc_SPUpper != upper)
    {
        printf("FAIL %s: this task's stack bounds changed\n", mode);
        return 0;
    }
    if (want_atexit && (GetVar((CONST_STRPTR)"stacktest_atexit", (STRPTR)flag, sizeof(flag), GVF_LOCAL_ONLY) != 1 || flag[0] != '1'))
    {
        printf("FAIL %s: the atexit() handler did not run\n", mode);
        return 0;
    }
    printf("PASS %s\n", mode);
    return 1;
}

int main(void)
{
    int i, ok = 1;

    seg = LoadSeg((CONST_STRPTR)"PROGDIR:stackchild");
    if (!seg)
    {
        printf("FAIL: PROGDIR:stackchild could not be loaded (error %ld)\n", (long)IoErr());
        return RETURN_FAIL;
    }
    for (i = 0; i < 2 && ok; i++)
    {
        ok = ok && run("return", 0, TRUE);
        ok = ok && run("exit", 7, TRUE);
    }
    cleanup();
    return ok ? RETURN_OK : RETURN_FAIL;
}
