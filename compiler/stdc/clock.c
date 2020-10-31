/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Returns time passed since start of program.
*/

#include <aros/symbolsets.h>
#include <proto/dos.h>

#include "__stdc_intbase.h"

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
        Reference point is set when stdc.library is opened.
        If you use the function from another shared library the reference
        point is thus when this library opened stdc.library

    EXAMPLE

    BUGS

    SEE ALSO
	time()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct DateStamp 	t;
    clock_t		retval;

    DateStamp(&t); /* Get timestamp */

    /* Day difference */
    retval =  (t.ds_Days - StdCBase->starttime.ds_Days);

    /* Convert into minutes */
    retval *= (24 * 60);

    /* Minute difference */
    retval += (t.ds_Minute - StdCBase->starttime.ds_Minute);

    /* Convert into CLOCKS_PER_SEC (which is the same as TICKS_PER_SECOND) units */
    retval *= (60 * TICKS_PER_SECOND);

    /* Add tick difference */
    retval += (t.ds_Tick - StdCBase->starttime.ds_Tick);

    return retval;
} /* clock */

int __init_clock(struct StdCIntBase *StdCBase)
{
    DateStamp(&StdCBase->starttime);

    return 1;
}

ADD2OPENLIB(__init_clock, 20);
