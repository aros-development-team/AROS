#ifndef FDSK_DEVICE_GCC_H
#define FDSK_DEVICE_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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
