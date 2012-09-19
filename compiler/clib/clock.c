/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Returns time passed since start of program.
*/

#include "__arosc_privdata.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>

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

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    return (clock_t)time(NULL) - aroscbase->acb_starttime;
} /* clock */

int __init_clock(struct aroscbase *aroscbase)
{
    aroscbase->acb_starttime = (clock_t)time(NULL);

    return 1;
}

ADD2OPENLIB(__init_clock, 20);
