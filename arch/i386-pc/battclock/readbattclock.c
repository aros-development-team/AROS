/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/
#include "battclock_intern.h"
#include "cmos.h"

static inline unsigned char read_port(unsigned char port);
static inline int bcd_to_dec(int x);

/*****************************************************************************

    NAME */
#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>

	AROS_LH0(ULONG, ReadBattClock,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct BattClockBase *, BattClockBase, 2, Battclock)

/*  FUNCTION
	Return the value stored in the battery back up clock. This value
	is the number of seconds that have elapsed since midnight on the
	1st of January 1978 (00:00:00 1.1.1978).

	If the value of the battery clock is invalid, then the clock will
	be reset.

    INPUTS

    RESULT
	The number of seconds since 1.1.1978 00:00:00

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WriteBattClock, ResetBattClock

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    battclock_lib.fd and clib/battclock_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    UWORD century;
    UWORD status_b;
    ULONG  secs;

    /* Make sure time isn't currently being updated */
    while ((read_port(STATUS_A) & 0x80) != 0);

    date.sec   = read_port(SEC);
    date.min   = read_port(MIN);
    date.hour  = read_port(HOUR);
    date.mday  = read_port(MDAY);
    date.month = read_port(MONTH);
    date.year  = read_port(YEAR);
    century    = read_port(CENTURY);
    status_b   = read_port(STATUS_B);

    if ((status_b & 0x04) == 0) {
	date.sec   = bcd_to_dec(date.sec);
	date.min   = bcd_to_dec(date.min);
	date.hour  = bcd_to_dec(date.hour);
	date.mday  = bcd_to_dec(date.mday);
	date.month = bcd_to_dec(date.month);
	date.year  = bcd_to_dec(date.year);
	century    = bcd_to_dec(century);
    }

    date.year = century * 100 + date.year;

    secs=Date2Amiga(&date);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */

/* help functions */
static inline unsigned char read_port(unsigned char port)
{
    unsigned char tmp;

    asm volatile (
    "outb	%1,$0x70	\n\t"		\
    "inb	$0x71,%0"			\
    : "=a"(tmp)				\
    : "a"(port));

    return tmp;
}

static inline int bcd_to_dec(int x)
{
    return ( (x >> 4) * 10 + (x & 0x0f) );
}
