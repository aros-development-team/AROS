/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef RAMDRIVE_DEVICE_GCC_H
#define RAMDRIVE_DEVICE_GCC_H

#include <aros/libcall.h>
#include <dos/bptr.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <exec/lists.h>

struct ramdrivebase
{
    struct Device 		device;
    BPTR                    seglist;
    struct DosLibrary       *dosbase;
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
    ULONG                   blocks;
    ULONG                   firstblock;
    ULONG                   tracks;
    ULONG                   dostype;
};

#endif
