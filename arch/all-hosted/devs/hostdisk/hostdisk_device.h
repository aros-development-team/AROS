#ifndef FDSK_DEVICE_H
#define FDSK_DEVICE_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <dos/dos.h>

struct HostDiskBase
{
    struct Device 		device;
    struct SignalSemaphore 	sigsem;
    struct MsgPort 		port;
    struct MinList 		units;
    struct HostInterface       *iface;
};

struct unit
{
    struct Message 		msg;
    struct HostDiskBase		*hdskBase;
    STRPTR                      filename;
    ULONG 			unitnum;
    ULONG			usecount;
    struct MsgPort 		port;
    file_t 			file;
    BOOL			writable;
    ULONG			changecount;
    struct MinList 		changeints;
};

file_t Host_Open(STRPTR name, struct HostInterrface *HostIf);
void Host_Close(file_t file, struct HostInterrface *HostIf);
LONG Host_Read(file_t file, APTR buf, ULONG size, ULONG *ioerr, struct HostInterrface *HostIf);
LONG Host_Write(file_t file, APTR buf, ULONG size, ULONG *ioerr, struct HostInterrface *HostIf);
BOOL Host_Seek(file_t file, ULONG pos, struct HostInterrface *HostIf);

#endif
