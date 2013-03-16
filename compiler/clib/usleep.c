/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2001 function usleep()
    Function is removed from POSIX.1-2008
*/

#include <aros/debug.h>

#include <time.h>

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
        This function is not part of POSIX.1-2008 anymore. Don't use this
        function. As an alternative nanosleep() can be used.

    EXAMPLE

    BUGS

    SEE ALSO
        nanosleep()

    INTERNALS
        This function is part of libarosc.a and may be removed in the future.

******************************************************************************/
{
    struct timespec req;

    req.tv_sec = usec/1000000;
    req.tv_nsec = (usec % 1000000)*1000;

    return nanosleep(&req, NULL);
} /* usleep() */

