#ifndef PIPEFS_HANDLER_GCC_H
#define PIPEFS_HANDLER_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase pipefsbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase pipefsbase->dosbase

#endif

