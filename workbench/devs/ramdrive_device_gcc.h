/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RAMDRIVE_DEVICE_GCC_H
#define RAMDRIVE_DEVICE_GCC_H

#include <aros/libcall.h>

struct ramdrivebase
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
    struct ramdrivebase 	*ramdrivebase;
    ULONG 			unitnum;
    ULONG			usecount;
    ULONG   	    	    	headpos;
    struct MsgPort 		port;
    UBYTE 			*mem;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct ramdrivebase *, ramdrivebase, 3, ramdrive)

#ifdef SysBase
    #undef SysBase
#endif
#ifdef DOSBase
    #undef DOSBase
#endif
#define SysBase ramdrivebase->sysbase
#define DOSBase ramdrivebase->dosbase

#endif
