#ifndef OS_AROS_H
#define OS_AROS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/macros.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <exec/interrupts.h>
#include <exec/types.h>
#include <aros/debug.h>
#include <proto/dos.h>
#include "afshandler.h"

#define OS_BE2LONG AROS_BE2LONG
#define OS_LONG2BE AROS_LONG2BE

#define OS_PTRALIGN AROS_PTRALIGN

struct IOHandle {
	STRPTR blockdevice;
	ULONG unit;
	ULONG flags;
	struct MsgPort *mp;
	struct IOExtTD *ioreq;
	struct IOExtTD *iochangeint;
	struct Interrupt mc_int;
	struct Interrupt vbl_int;
	struct AFSBase *afsbase; /* for interrupt code */
	ULONG ioflags;
	UWORD cmdread;
	UWORD cmdwrite;
	UWORD cmdseek;
	UWORD cmdformat;
	UBYTE moff_time;
};

#define IOHF_MOTOR_OFF    (1<<0)
#define IOHF_MEDIA_CHANGE (1<<1)
#define IOHF_DISK_IN      (1<<2)
#define IOHF_TRACKDISK    (1<<3)

void checkDeviceFlags(struct AFSBase *);

#endif
