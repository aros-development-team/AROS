#ifndef FDSK_DEVICE_GCC_H
#define FDSK_DEVICE_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <dos/dos.h>

struct fdskbase
{
    struct Device 		device;
    struct ExecBase 		*sysbase;
    APTR 			seglist;
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

#endif
