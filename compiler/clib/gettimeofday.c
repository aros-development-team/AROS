/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Query the current time and/or timezone.
*/

#include "__arosc_privdata.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>


#include "__time.h"

long __gmtoffset;

/*****************************************************************************

    NAME */
#include <sys/time.h>
#include <unistd.h>

	int gettimeofday (

/*  SYNOPSIS */
	struct timeval	* tv,
	struct timezone * tz)

/*  FUNCTION
	Return the current time and/or timezone.

    INPUTS
	tv - If this pointer is non-NULL, the current time will be
		stored here. The structure looks like this:

		struct timeval
		{
		    long tv_sec;	// seconds
		    long tv_usec;	// microseconds
		};

	tz - If this pointer is non-NULL, the current timezone will be
		stored here. The structure looks like this:

		struct timezone
		{
		    int  tz_minuteswest; // minutes west of Greenwich
		    int  tz_dsttime;	 // type of dst correction
		};

		With daylight savings times defined as follows :

		DST_NONE	// not on dst
		DST_USA 	// USA style dst
		DST_AUST	// Australian style dst
		DST_WET 	// Western European dst
		DST_MET 	// Middle European dst
		DST_EET 	// Eastern European dst
		DST_CAN 	// Canada
		DST_GB		// Great Britain and Eire
		DST_RUM 	// Rumania
		DST_TUR 	// Turkey
		DST_AUSTALT	// Australian style with shift in 1986

		And the following macros are defined to operate on this :

		timerisset(tv) - TRUE if tv contains a time

		timercmp(tv1, tv2, cmp) - Return the result of the
			comparison "tv1 cmp tv2"

		timerclear(tv) - Clear the timeval struct

    RESULT
	The number of seconds.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	struct timeval tv;

	// Get the current time and print it
	gettimeofday (&tv, NULL);

	printf ("Seconds = %ld, uSec = %ld\n", tv->tv_sec, tv->tv_usec);

    BUGS

    SEE ALSO
	ctime(), asctime(), localtime(), time()

    INTERNALS

******************************************************************************/
{
    if (tv)
    {
        GetSysTime(tv);

        /* Adjust with the current timezone, stored in minutes west of GMT */
        tv->tv_sec += __gmtoffset * 60;
    }

    if (tz)
    {
	tz->tz_minuteswest = __gmtoffset;
#warning FIXME: set tz->tz_dsttime
	tz->tz_dsttime	   = DST_NONE;
    }

    return 0;
} /* gettimeofday */

static int __init_timerbase(void)
{
    __timeport.mp_Node.ln_Type   = NT_MSGPORT;
    __timeport.mp_Node.ln_Pri    = 0;
    __timeport.mp_Node.ln_Name   = NULL;
    __timeport.mp_Flags          = PA_IGNORE;
    __timeport.mp_SigTask        = FindTask(NULL);
    __timeport.mp_SigBit         = 0;
    NEWLIST(&__timeport.mp_MsgList);

    __timereq.tr_node.io_Message.mn_Node.ln_Type    = NT_MESSAGE;
    __timereq.tr_node.io_Message.mn_Node.ln_Pri     = 0;
    __timereq.tr_node.io_Message.mn_Node.ln_Name    = NULL;
    __timereq.tr_node.io_Message.mn_ReplyPort       = &__timeport;
    __timereq.tr_node.io_Message.mn_Length          = sizeof (__timereq);

    if
    (
        OpenDevice
        (
            "timer.device",
            UNIT_VBLANK,
            (struct IORequest *)&__timereq,
            0
        )
        ==
        0
    )
    {
        TimerBase = (struct Device *)__timereq.tr_node.io_Device;
        return 1;
    }
    else
    {
        return 0;
    }
}


static void __exit_timerbase(void)
{
    if (TimerBase != NULL)
        CloseDevice((struct IORequest *)&__timereq);
}

ADD2INIT(__init_timerbase, 0);
ADD2EXIT(__exit_timerbase, 0);
