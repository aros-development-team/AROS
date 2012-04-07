/*
    Copyright © 2008-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function nanosleep().
*/

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

    EXAMPLE

    BUGS

    SEE ALSO
	
    INTERNALS
	sorry, this function just calls usleep()

******************************************************************************/
{
    useconds_t usec = req->tv_sec * 10000000 + req->tv_nsec / 1000;
    return usleep(usec);
} /* nanosleep() */

