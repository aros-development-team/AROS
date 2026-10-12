#ifndef OS_AROS_H
#define OS_AROS_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/utility.h>
#include <proto/dos.h>

#include <aros/macros.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/stdio.h>
#include <exec/interrupts.h>
#include <exec/types.h>
#include <aros/debug.h>

#include "error.h"
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
	ULONG sectorsize;
	ULONG ioflags;
	UWORD cmdread;
	UWORD cmdwrite;
	UWORD cmdseek;
	UWORD cmdformat;
};

#define IOHF_MOTOR_OFF    (1<<0)
#define IOHF_MEDIA_CHANGE (1<<1)
#define IOHF_DISK_IN      (1<<2)

enum showReqType;

LONG showPtrArgsText(struct AFSBase *afsbase, const char *string, enum showReqType type, RAWARG args);
LONG showError(struct AFSBase *afsbase, ULONG error, ...);

/* Generic clients retain inline wrappers; constrained builds can select
 * one shared implementation through their target build rules. */
#ifndef AFS_INLINE_REQUESTERS
#define AFS_INLINE_REQUESTERS 1
#endif

#if AFS_INLINE_REQUESTERS
#define AFS_REQUESTER_LINKAGE static inline
#include "os_aros_requesters.h"
#undef AFS_REQUESTER_LINKAGE
#else
LONG showText(struct AFSBase *afsbase, const char *format, ...);
LONG showRetriableError(struct AFSBase *afsbase, const char *format, ...);
#endif

void checkDeviceFlags(struct AFSBase *);
void motorOff(struct AFSBase *afsbase, struct IOHandle *ioh);

#endif
