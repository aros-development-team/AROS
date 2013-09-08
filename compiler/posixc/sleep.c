/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
    
    POSIX.1-2008 function sleep()
*/
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	unsigned int sleep (

/*  SYNOPSIS */
	unsigned int seconds )

/*  FUNCTION
	The sleep() function makes the current process sleep for the
	specified number of seconds or until a signal arrives which
	is not ignored.

    INPUTS
	seconds - The number of seconds to sleep

    RESULT
    	Zero if the requested time has elapsed, or the number of seconds
	left to sleep when the process was signalled.

    NOTES

    EXAMPLE
    	// Sleep for 10 seconds
	sleep( 10 );
	
    BUGS
	The current implementation simply uses the dos.library function
	Delay() to sleep, and cannot be interrupted by incoming signals.
	This shouldn't be of any importance, since AROS doesn't have 
	POSIX style signalling yet (but when it is implemented, this
	function needs to be changed). 

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    Delay( seconds * 50 );
    
    return 0;
} /* sleep */
