/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/23 17:32:23  digulla
    Implementation of the console.device


    Desc:
    Lang:
*/
#ifndef CONSOLE_GCC_H
#define CONSOLE_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct consolebase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
};

#define expunge() \
__AROS_LC0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase ConsoleDevice->sysBase

#endif

