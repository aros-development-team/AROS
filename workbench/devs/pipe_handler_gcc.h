/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.1  2001/07/15 00:11:30  falemagn
    pipe.handler. It supports multiple writers and readers. soon it will support named pipes and the pipe() syscall. There's no buffering, at the moment.

    Desc:
    Lang:
*/
#ifndef PIPE_HANDLER_GCC_H
#define PIPE_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct pipebase
{
    struct Device      device;
    struct ExecBase   *sysbase;
    struct DosLibrary *dosbase;
    struct Process    *proc;
    BPTR               seglist;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct pipebase *, pipebase, 3, pipe_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase pipebase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase pipebase->dosbase

#endif

