/*
    Copyright (C) 2010-2023, The AROS Development Team. All rights reserved.

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

#include <aros/debug.h>

#include "debug.h"

/* For each opener stdin, stdout and stderr may be different */
static int __init_stdio(struct StdCIOIntBase *StdCIOBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR _in, _out;

    NewList((struct List *)&StdCIOBase->files);

    _in = Input();
    _out = Output();

    StdCIOBase->intstdin.fh = DupFileHandle(_in);
    StdCIOBase->intstdin.flags =
        __STDCIO_STDIO_READ
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE
        | __STDCIO_STDIO_FLUSHONREAD;
    D(bug("[%s] %s: intstdin.fh = 0x%p\n", STDCNAME, __func__, StdCIOBase->intstdin.fh));
    StdCIOBase->StdCIOBase._stdin = &StdCIOBase->intstdin;

    StdCIOBase->intstdout.fh = DupFileHandle(_out);
    StdCIOBase->intstdout.flags =
        __STDCIO_STDIO_WRITE
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE;
    D(bug("[%s] %s: intstdout.fh = 0x%p\n", STDCNAME, __func__, StdCIOBase->intstdout.fh));
    StdCIOBase->StdCIOBase._stdout = &StdCIOBase->intstdout;

    StdCIOBase->intstderr.fh = me->pr_CES ? me->pr_CES : me->pr_COS;
    StdCIOBase->intstderr.flags =
        __STDCIO_STDIO_WRITE
        | __STDCIO_STDIO_DONTCLOSE
        | __STDCIO_STDIO_DONTFREE;
    D(bug("[%s] %s: intstderr.fh = 0x%p\n", STDCNAME, __func__, StdCIOBase->intstderr.fh));
    StdCIOBase->StdCIOBase._stderr = &StdCIOBase->intstderr;

    return 1;
}


static int __close_stdio(struct StdCIOIntBase *StdCIOBase)
{
    FILE *stream;
    BPTR _in, _out;

    D(bug("[%s] %s: StdCIOBase = 0x%p, DOSBase = 0x%p\n", STDCNAME, __func__, StdCIOBase, DOSBase));

    _in = Input();
    _out = Output();

    if (StdCIOBase->intstdin.fh != _in)
        Close(StdCIOBase->intstdin.fh);
    if (StdCIOBase->intstdout.fh != _out)
        Close(StdCIOBase->intstdout.fh);

    ForeachNode(&StdCIOBase->files, stream)
    {
        D(bug("[%s] %s: stream @ 0x%p, fh = 0x%p\n", STDCNAME, __func__, stream, stream->fh));
        stream->flags |= __STDCIO_STDIO_DONTFREE;
        fclose(stream);
    }

    if (StdCIOBase->streampool)
        DeletePool(StdCIOBase->streampool);

    return 1;
}

ADD2OPENLIB(__init_stdio, 0)
ADD2CLOSELIB(__close_stdio, 0)
