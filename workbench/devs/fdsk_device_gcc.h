/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.3  2000/10/11 17:16:18  stegerg
    The Unit process entry function (used with CreateNewProc(NP_Entry))
    had missing arguments which caused crashes. It had just "SysBase"
    as argument, but it must also have argstr and arglen. This would
    not be a problem on 68k where this params are in registers, but
    it does not work with stack params as on x86, because SysBase is
    not the first arg, but the third.

    Source Cleanup + small fixes + added debug output.

    Revision 1.2  1998/10/20 16:47:24  hkiel
    Amiga Research OS

    Revision 1.1  1996/11/14 08:53:29  aros
    First attempt for a real fastfilesystem
    (only directoryscans for now)


    Desc:
    Lang:
*/
#ifndef FDSK_DEVICE_GCC_H
#define FDSK_DEVICE_GCC_H
#include <aros/libcall.h>

struct fdskbase
{
    struct Device 		device;
    struct ExecBase 		*sysbase;
    struct DosLibrary 		*dosbase;
    BPTR 			seglist;
    struct SignalSemaphore 	sigsem;
    struct MsgPort 		port;
    struct MinList 		units;
};

struct unit
{
    struct Message 		msg;
    struct fdskbase 		*fdskbase;
    ULONG 			unitnum;
    ULONG			usecount;
    struct MsgPort 		port;
    BPTR 			file;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct fdskbase *, fdskbase, 3, ram)

#ifdef SysBase
    #undef SysBase
#endif
#ifdef DOSBase
    #undef DOSBase
#endif
#define SysBase fdskbase->sysbase
#define DOSBase fdskbase->dosbase

#endif
