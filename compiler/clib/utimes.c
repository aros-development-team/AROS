/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <sys/time.h>
#include <errno.h>

#include "__upath.h"

/*****************************************************************************

    NAME */

	int utimes(
/*  SYNOPSIS */

	const char *file,
	struct timeval tvp[2])

/*  FUNCTION
	Change last access and last modification time of the given file to
	times specified in tvp array. If tvp is NULL, the current time will be
	used instead.

    INPUTS
	filename - Name of the file
	buf - Pointer to an array of two timeval structures. First structure
		specifies the last access time, second specifies the last
		modification time

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES
	The timeval structure has microsecond resolution, but in reality
	this function has time resolution of 1 tick.

    EXAMPLE

    BUGS
	Since AROS has no notion of last access time, it's silently ignored
	and only modification time of the file is set.

    SEE ALSO
	utime()

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
	ULONG t = (ULONG)tvp[1].tv_sec - 2922 * 1440 * 60;

	ds.ds_Days   = t / (60*60*24);
	ds.ds_Minute = (t / 60) % (60*24);
	ds.ds_Tick   = (t % 60) * TICKS_PER_SECOND;
    }
    else
	DateStamp(&ds);

    if (SetFileDate(file, &ds))
	return 0;
    else
	errno = __arosc_ioerr2errno(IoErr());

    return -1;
}
