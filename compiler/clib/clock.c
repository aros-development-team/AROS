/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Returns time passed since start of program
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>

#ifndef _CLIB_KERNEL_
struct DateStamp __startup_datestamp;
#endif

/*****************************************************************************

    NAME */
#include <time.h>

	clock_t clock (

/*  SYNOPSIS */
	void)

/*  FUNCTION
       clock() returns an approximation of the time passed since
       the program was started

    INPUTS

    RESULT
	The time passed in CLOCKS_PER_SEC units. To get the
	number of seconds divide by CLOCKS_PER_SEC.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	time()

    INTERNALS

    HISTORY
	15.12.2000 stegerg created

******************************************************************************/
{
    GETUSER;

    struct DateStamp 	t;
    clock_t		retval;

    DateStamp (&t); /* Get timestamp */

    /* Day difference */
    retval =  (t.ds_Days - __startup_datestamp.ds_Days);

    /* Convert into minutes */
    retval *= (24 * 60);

    /* Minute difference */
    retval += (t.ds_Minute - __startup_datestamp.ds_Minute);

    /* Convert into CLOCKS_PER_SEC (which is the same as TICKS_PER_SECOND) units */
    retval *= (60 * TICKS_PER_SECOND);

    /* Add tick difference */
    retval += (t.ds_Tick - __startup_datestamp.ds_Tick);

    return retval;

} /* clock */

int __init_clock(void)
{
    GETUSER;

    DateStamp(&__startup_datestamp);
    return 0;
}

ADD2INIT(__init_clock, 20);
