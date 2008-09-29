/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <sys/time.h>
#include <errno.h>

#include "__errno.h"
#include "__upath.h"

/*****************************************************************************

    NAME */

	int utimes(
/*  SYNOPSIS */

	const char *file,
	struct timeval tvp[2])

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
    struct DateStamp ds;

    if (!file) /*safety check */
    {
    	errno = EFAULT;
	return -1;
    }

    file = __path_u2a(file);
    if (!file)
        return -1;

    if(tvp != NULL)
    {   
	ULONG t = (ULONG)tvp[1].tv_sec;

	ds.ds_Days   = t / (60*60*24);
	ds.ds_Minute = (t / 60) % (60*24);
	ds.ds_Tick   = (t % 60) * TICKS_PER_SECOND;
    }
    else
	DateStamp(&ds);

    if (SetFileDate(file, &ds))
	return 0;
    else
	errno = IoErr2errno(IoErr());

    return -1;
}
