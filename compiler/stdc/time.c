/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Return the current time in seconds.
*/

#include <proto/exec.h>
#include <proto/timer.h>

#include <errno.h>

#include "__stdc_intbase.h"

#define DEBUG 0
#include <aros/debug.h>

static int __init_timerbase(struct StdCIntBase *StdCBase);
struct Device *TimerBase = NULL;

/*****************************************************************************

    NAME */
#include <time.h>

	time_t time (

/*  SYNOPSIS */
	time_t * tloc)

/*  FUNCTION
       time() returns the time since 00:00:00 GMT, January 1, 1970,
       measured in seconds.

    INPUTS
	tloc - If this pointer is non-NULL, then the time is written into
		this variable as well.

    RESULT
	The number of seconds.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	time_t tt1, tt2;

	// tt1 and tt2 are the same
	tt1 = time (&tt2);

	// This is valid, too
	tt1 = time (NULL);

    BUGS

    SEE ALSO
	ctime(), asctime(), localtime()

    INTERNALS

******************************************************************************/
{
    struct timeval tv;

    /* We get TimerBase here and not during LIBINIT because timer.device is not available
       when stdc.library is initialized.
    */
    if (TimerBase == NULL)
        __init_timerbase((struct StdCIntBase *)__aros_getbase_StdCBase());
    if (TimerBase == NULL)
    {
        errno = EACCES;
        return (time_t)-1;
    }

    GetSysTime(&tv);
    tv.tv_sec += 2922 * 1440 * 60;

    tv.tv_sec += __stdc_gmtoffset() * 60;

    if (tloc)
        *tloc = tv.tv_sec;
    return tv.tv_sec;
} /* time */


static int __init_timerbase(struct StdCIntBase *StdCBase)
{
    D(bug("__init_timerbase\n"));

    StdCBase->timeport.mp_Node.ln_Type   = NT_MSGPORT;
    StdCBase->timeport.mp_Node.ln_Pri    = 0;
    StdCBase->timeport.mp_Node.ln_Name   = NULL;
    StdCBase->timeport.mp_Flags          = PA_IGNORE;
    StdCBase->timeport.mp_SigTask        = FindTask(NULL);
    StdCBase->timeport.mp_SigBit         = 0;
    NEWLIST(&StdCBase->timeport.mp_MsgList);

    StdCBase->timereq.tr_node.io_Message.mn_Node.ln_Type    = NT_MESSAGE;
    StdCBase->timereq.tr_node.io_Message.mn_Node.ln_Pri     = 0;
    StdCBase->timereq.tr_node.io_Message.mn_Node.ln_Name    = NULL;
    StdCBase->timereq.tr_node.io_Message.mn_ReplyPort       = &StdCBase->timeport;
    StdCBase->timereq.tr_node.io_Message.mn_Length          = sizeof (StdCBase->timereq);

    if (OpenDevice(
            "timer.device", UNIT_VBLANK,
            (struct IORequest *)&StdCBase->timereq, 0
        ) == 0
    )
    {
        TimerBase = (struct Device *)StdCBase->timereq.tr_node.io_Device;
        D(bug("__init_timerbase TimerBase=%x\n", TimerBase));
        return 1;
    }
    else
    {
        D(bug("__init_timerbase OpenDevice failed\n"));
        return 0;
    }
}


static void __exit_timerbase(struct StdCIntBase *StdCBase)
{
    D(bug("__exit_timerbase\n"));

    if (TimerBase != NULL)
    {
        CloseDevice((struct IORequest *)&StdCBase->timereq);
        TimerBase = NULL;
    }
}

ADD2EXPUNGELIB(__exit_timerbase, 0);
