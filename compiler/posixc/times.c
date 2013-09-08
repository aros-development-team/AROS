/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function times().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <sys/times.h>

	clock_t times(

/*  SYNOPSIS */
	struct tms *tms)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement times() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = ENOSYS;
    
    tms->tms_utime  = 0;
    tms->tms_stime  = 0;
    tms->tms_cutime = 0;
    tms->tms_cstime = 0;
    
    return -1;
}

