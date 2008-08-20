#ifndef AFS_HANDLER_H
#define AFS_HANDLER_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: afshandler.h 27106 2007-10-28 10:49:03Z verhaegs $
*/

#include <exec/devices.h>
#include <dos/filesystem.h>
#include <devices/timer.h>

struct AFSBase
{
	struct Device device;
	struct IntuitionBase *intuitionbase;
	struct DosLibrary *dosbase;
	struct MsgPort port;			/* MsgPort of the handler */
	struct MsgPort rport;		/* replyport of the handler */
	struct IOFileSys *iofs;		/* to be aborted iofs or NULL */
	struct List device_list;	/* list of mounted devices (struct Volume) */
	struct timerequest *timer_request;
	ULONG timer_flags;
};

#define TIMER_ACTIVE  0x00000001
#define TIMER_RESTART 0x00000002

#endif
