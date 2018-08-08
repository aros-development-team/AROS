/*
    Copyright © 2008-2018, The AROS Development Team. All rights reserved.
    $Id$
    POSIX.1-2001 function clock_gettime().
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <errno.h>

#include "__posixc_time.h"

/*****************************************************************************
    NAME */
#include <time.h>

    int clock_gettime (

/*  SYNOPSIS */
    clockid_t clk_id,
    struct timespec *tp)
        
/*  FUNCTION
        retrieve the time of the specified clock clk_id.

    INPUTS
        clk_id - identifier of the particular clock on which to act
                CLOCK_REALTIME 
                System-wide realtime clock. Setting this clock requires appropriate privileges. 
                CLOCK_MONOTONIC 
                Clock that cannot be set and represents monotonic time since some unspecified starting point. 
                CLOCK_PROCESS_CPUTIME_ID 
                High-resolution per-process timer from the CPU. 
                CLOCK_THREAD_CPUTIME_ID 
                Thread-specific CPU-time clock. 
        tp - structure to hold the retrieved time value

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
    struct PosixCIntBase *PosixCBase = (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int retval = -1;

    if (!TimerBase)
        __init_timerbase(PosixCBase);

    switch(clk_id)
    {
        case CLOCK_REALTIME:
        {
            if (TimerBase)
            {
                struct timeval tv;
                GetSysTime(&tv);
                tp->tv_sec  = tv.tv_sec;
                tp->tv_nsec = tv.tv_usec * 1000; 
                retval = 0;
            }
            else
            {
                errno = EACCES;
            }
            break;
        }

        case CLOCK_MONOTONIC:
        {
            if (TimerBase)
            {
                struct timeval tv;
                GetUpTime(&tv);
                tp->tv_sec  = tv.tv_sec;
                tp->tv_nsec = tv.tv_usec * 1000; 
                retval = 0;
            }
            else
            {
                errno = EACCES;
            }
            break;
        }

        default:
            errno = EINVAL;
            break;
    }


    return retval;
} /* clock_gettime() */
