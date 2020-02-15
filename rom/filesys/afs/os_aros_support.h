#ifndef OS_AROS_H
#define OS_AROS_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
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

static inline LONG showText(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_Cancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}

static inline LONG showRetriableError(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_RetryCancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}

void checkDeviceFlags(struct AFSBase *);
void motorOff(struct AFSBase *afsbase, struct IOHandle *ioh);

#endif
