/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$
    $Log$
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
    struct MsgPort *replyport;
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

