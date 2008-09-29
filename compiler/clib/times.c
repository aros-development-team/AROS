/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function times().
*/

#include <aros/debug.h>

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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#   warning Implement times()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    tms->tms_utime  = 0;
    tms->tms_stime  = 0;
    tms->tms_cutime = 0;
    tms->tms_cstime = 0;
    
    return -1;
}

