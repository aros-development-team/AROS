/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ZERO: handler
    Lang: English
*/

#ifndef ZERO_HANDLER_GCC_H
#define ZERO_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct zerobase
{
    struct Device device;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    BPTR seglist;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct zerobase *, zerobase, 3, zero_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase zerobase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase zerobase->dosbase

#endif

