/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Return the number of seconds elapsed between time2 and time1.
*/

/*****************************************************************************

    NAME */
#ifndef AROS_NOFPU
#include <time.h>

	double difftime (

/*  SYNOPSIS */
	time_t time2,
	time_t time1)

/*  FUNCTION
       difftime() returns the number of seconds elapsed between
       time time2 and time time1. 

    INPUTS
	time2 - time value from which time1 is subtracted
	time1 - time value that is subtracted from time2

    RESULT
	The number of seconds elapsed in double precision.

    NOTES

    EXAMPLE
	time_t tt1, tt2;
	double secs;
	
	time (&tt1);
        ...
	time (&tt2);
	
	secs = difftime(tt2, tt1);
	
    BUGS

    SEE ALSO
	time(), ctime(), asctime(), localtime()

    INTERNALS

******************************************************************************/
{
    return (double)(time2 - time1);
    
} /* difftime */

#else

void difftime(void)
{
	return;
}

#endif /* AROS_NOFPU */
