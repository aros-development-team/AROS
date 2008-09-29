/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <time.h>
#include <sys/timeb.h>

	int ftime(

/*  SYNOPSIS */
	struct timeb *tb)

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
    tb->time     = time(NULL);
    tb->millitm  = 0; // FIXME
    tb->timezone = 0; // FIXME
    tb->dstflag  = 0; // FIXME

    return 0;
}

