/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix-based implementation of DateStamp()
    Lang: english
*/
#include <aros/system.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <aros/host-conf.h>

#define timeval sys_timeval
#include <sys/time.h>
#include <stdlib.h>
#undef timeval
#include "dos_intern.h"

#define SECONDS_PER_DAY     (60UL * 60 * 24)
#define SECONDS_PER_MINUTE  (60UL)
#define uSEC_PER_SEC	    (1000000UL)
#define TICKS_PER_SEC	    (50UL)
#define uSEC_PER_TICK	    (uSEC_PER_SEC / TICKS_PER_SEC)
#define AMIGA_UNIX_OFFSET   (2922UL) /* Days between 1.1.78 (Amiga) and 1.1.70 (Unix) */

AROS_LH1(struct DateStamp *, DateStamp,
	AROS_LHA(struct DateStamp *, date, D1),
	struct DosLibrary *, DOSBase, 32, Dos)
{
    AROS_LIBFUNC_INIT

    struct sys_timeval stv;
    gettimeofday (&stv, NULL);

    date->ds_Days = stv.tv_sec / SECONDS_PER_DAY - AMIGA_UNIX_OFFSET;
    stv.tv_sec %= SECONDS_PER_DAY;
    date->ds_Minute = stv.tv_sec / SECONDS_PER_MINUTE;
    stv.tv_sec %= SECONDS_PER_MINUTE;
    date->ds_Tick = (stv.tv_usec + stv.tv_sec * uSEC_PER_SEC) /
	    uSEC_PER_TICK;

    return date;
    AROS_LIBFUNC_EXIT
} /* DateStamp */
