/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.2  2001/07/15 20:16:38  falemagn
    Implemented named pipes. Actually there are ONLY named pipes. The standard AmigaOS PIPE: can be implemented assigning PIPE: to PIPEFS:namedpipe. pipe() support is about to come

    Revision 1.1  2001/07/15 00:11:30  falemagn
    pipefs.handler. It supports multiple writers and readers. soon it will support named pipes and the pipe() syscall. There's no buffering, at the moment.

    Desc:
    Lang:
*/
#ifndef PIPEFS_HANDLER_GCC_H
#define PIPEFS_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct pipefsbase
{
    struct Device      device;
    struct ExecBase   *sysbase;
    struct DosLibrary *dosbase;
    struct Process    *proc;
    BPTR               seglist;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct pipefsbase *, pipefsbase, 3, pipefs_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase pipefsbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase pipefsbase->dosbase

#endif

