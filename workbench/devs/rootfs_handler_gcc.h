/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.2  2001/06/11 15:28:44  falemagn
    removed a bug. I tried to implement directory listing, but it came out to be more troubling that I expected. Anyway... listing is not mandatory, it serves its purpouses the way it is now. I'll look at it some other days.

    Revision 1.1  2001/06/10 21:49:47  falemagn
    A virtual filesystem that emulates the unixish root directory. Doesn't allow listing yet. It serves to emulate in a clean way the unix paths


    Desc: Header file for a virtual filesystem that emulates the unixish root dir
    Lang: English
*/

#ifndef ROOTFS_HANDLER_GCC_H
#define ROOTFS_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct rootfsbase
{
    struct Device device;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    BPTR seglist;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct rootfsbase *, rootfsbase, 3, rootfs_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase rootfsbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase rootfsbase->dosbase

#endif

