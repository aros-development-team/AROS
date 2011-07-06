#ifndef AFS_HANDLER_H
#define AFS_HANDLER_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/devices.h>
#include <devices/timer.h>

struct AFSBase
{
	struct Library ab_Lib;
	BPTR   ab_SegList;		/* Pointer to SegList trampoline */
	struct Volume *volume;
	struct DosLibrary *dosbase;
	struct MsgPort *timer_mp;
	struct MsgPort port;	/* sigtask and sigbit for changeint */
	struct List device_list;	/* list of mounted devices (struct Volume) */
	struct timerequest *timer_request;
	ULONG timer_flags;
};

#define TIMER_ACTIVE  0x00000001
#define TIMER_RESTART 0x00000002

#endif
