/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/syscall.h>
#include <sys/arosc.h>

int do_arosc_internals __attribute__((weak)) = 0;

static int postopen(void)
{
    return do_arosc_internals ? syscall(SYS_arosc_internalinit, NULL) : 0;
}

static void preclose(void)
{
    if (do_arosc_internals)
        syscall(SYS_arosc_internalexit);
}

ADD2LIBS("arosc.library", 39, LIBSET_AROSC_PRI, struct Library *, aroscbase, postopen, preclose);
