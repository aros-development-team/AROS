/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Setup and support code for stdio.h functionality
*/
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "__stdio.h"
#include "__stdcio_intbase.h"

#define DEBUG 0
#include <aros/debug.h>

/* For each opener stdin, stdout and stderr may be different */
static int __init_stdio(struct StdCIOIntBase *StdCIOBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    NewList((struct List *)&StdCIOBase->files);

    StdCIOBase->intstdin.fh = Input();
    StdCIOBase->intstdin.flags =
        __STDCIO_STDIO_READ
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE
        | __STDCIO_STDIO_FLUSHONREAD;
    D(bug("[__init_stdio]: intstdin.fh=0x%x\n", StdCIOBase->intstdin.fh));
    StdCIOBase->StdCIOBase._stdin = &StdCIOBase->intstdin;

    StdCIOBase->intstdout.fh = Output();
    StdCIOBase->intstdout.flags =
        __STDCIO_STDIO_WRITE
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE;
    D(bug("[__init_stdio]: intstdout.fh=0x%x\n", StdCIOBase->intstdout.fh));
    StdCIOBase->StdCIOBase._stdout = &StdCIOBase->intstdout;

    StdCIOBase->intstderr.fh = me->pr_CES ? me->pr_CES : me->pr_COS;
    StdCIOBase->intstderr.flags =
        __STDCIO_STDIO_WRITE
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE;
    D(bug("[__init_stdio]: intstderr.fh=0x%x\n", StdCIOBase->intstderr.fh));
    StdCIOBase->StdCIOBase._stderr = &StdCIOBase->intstderr;

    return 1;
}


static int __close_stdio(struct StdCIOIntBase *StdCIOBase)
{
    FILE *stream;

    D(bug("[__close_stdio]: StdCIOBase: %x, DOSBase: %x\n", StdCIOBase, DOSBase));

    ForeachNode(&StdCIOBase->files, stream)
    {
        D(bug("[__close_stdio]: stream: %x, fh: %x\n", stream, stream->fh));
        stream->flags |= __STDCIO_STDIO_DONTFREE;
        fclose(stream);
    }

    if (StdCIOBase->streampool)
        DeletePool(StdCIOBase->streampool);

    return 1;
}

ADD2OPENLIB(__init_stdio, 0)
ADD2CLOSELIB(__close_stdio, 0)
