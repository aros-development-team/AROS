/*
    Copyright © 2008-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function nanosleep().
*/
#include <exec/exec.h>
#include <proto/exec.h>
#include <devices/timer.h>

#include <unistd.h>

/*****************************************************************************

    NAME */
#include <time.h>

	int nanosleep (

/*  SYNOPSIS */
	const struct timespec * req, struct timespec *rem)
        
/*  FUNCTION
        Suspends program execution for a given number of nanoseconds.

    INPUTS
        req - time to wait
	rem - remaining time, if nanosleep was interrupted by a signal

    RESULT
        0 on success, -1 on error

    NOTES
        Currently at most a resolution of milliseconds is supported.

    EXAMPLE

    BUGS

    SEE ALSO
	
    INTERNALS

******************************************************************************/
{
    struct MsgPort      *timerMsgPort;
    struct timerequest  *timerIO;
    int retval = -1;
    
    /* FIXME: share TimerBase with gettimeofday and don't open/close it for each usleep call */
    if((timerMsgPort = CreateMsgPort()))
    {
	timerIO = (struct timerequest *) CreateIORequest(timerMsgPort, sizeof (struct timerequest));
	if(timerIO)
	{
	    if(OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) timerIO, 0) == 0)
	    {
		timerIO->tr_node.io_Command = TR_ADDREQUEST;
		timerIO->tr_time.tv_secs    = req->tv_sec;
		timerIO->tr_time.tv_micro   = (req->tv_nsec+500)/1000;
  
		DoIO((struct IORequest *) timerIO);
		retval = 0;

                if (rem)
                {
                    rem->tv_sec = 0;
                    rem->tv_nsec = 0;
                }

		CloseDevice((struct IORequest *) timerIO);
	    }
	    DeleteIORequest((struct IORequest *) timerIO);
	}
	DeleteMsgPort(timerMsgPort);
    }
    return retval;
} /* nanosleep() */

