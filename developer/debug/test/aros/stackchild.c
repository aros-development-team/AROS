/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Helper for the "stack" test: declares a minimum stack with __stack and is
    run by the test through RunCommand() with a smaller stack.

    Usage: stackchild <task> return|exit
    <task> is the address of the running task as the test saw it (RunCommand
    runs this program on the caller's task). Return codes: 0 the checks passed
    and main() returned, 7 the checks passed and exit(7) was called from deep
    in a recursion, 20 a check failed.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/tasks.h>
#include <dos/var.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_STACK (512 * 1024)

ULONG __stack = MIN_STACK;

static void mark_atexit(void)
{
    SetVar((CONST_STRPTR)"stacktest_atexit", (CONST_STRPTR)"1", 1, GVF_LOCAL_ONLY);
}

/* Uses about 1 KB of stack per level; exits at level 0 when asked to. */
static int deep(int level, int do_exit)
{
    volatile unsigned char buf[1024];

    memset((void *)buf, level & 0xff, sizeof(buf));
    if (level == 0)
    {
        if (do_exit)
            exit(7);
        return buf[0];
    }
    return deep(level - 1, do_exit) + buf[1] - (level & 0xff);
}

int main(int argc, char **argv)
{
    struct Task *me = FindTask(NULL);
    IPTR have = (IPTR)me->tc_SPUpper - (IPTR)me->tc_SPLower;
    int do_exit;

    if (argc != 3)
        return 20;
    do_exit = !strcmp(argv[2], "exit");

    if ((IPTR)me != (IPTR)strtoull(argv[1], NULL, 16))
    {
        printf("stackchild: running on task %p, not on the caller's task %s\n", me, argv[1]);
        return 20;
    }
    if (have < MIN_STACK)
    {
        printf("stackchild: %lu bytes of stack, __stack asks for %lu\n", (unsigned long)have, (unsigned long)MIN_STACK);
        return 20;
    }
    if (atexit(mark_atexit) != 0)
        return 20;

    /* 400 levels of about 1 KB: more than the caller's stack, less than __stack. */
    if (deep(400, do_exit) != 0)
        return 20;
    return 0;
}
