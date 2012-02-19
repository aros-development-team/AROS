/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function usleep().
*/

#include <aros/debug.h>

#include <exec/exec.h>
#include <proto/exec.h>
#include <devices/timer.h>

#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int usleep (

/*  SYNOPSIS */
	useconds_t usec) 
        
/*  FUNCTION
        Suspends program execution for a given number of microseconds.

    INPUTS
        usec - number of microseconds to wait

    RESULT
        0 on success, -1 on error

    NOTES

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
		timerIO->tr_time.tv_secs    = 0;
		timerIO->tr_time.tv_micro   = usec;
  
		DoIO((struct IORequest *) timerIO);
		retval = 0;

		CloseDevice((struct IORequest *) timerIO);
	    }
	    DeleteIORequest((struct IORequest *) timerIO);
	}
	DeleteMsgPort(timerMsgPort);
    }
    return retval;
} /* usleep() */

