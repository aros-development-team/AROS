/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>
#include <time.h>
#include <sys/time.h>

/*****************************************************************************

    NAME */

#include <utime.h>

	int utime(

/*  SYNOPSIS */
	const char *filename,
	const struct utimbuf *buf)

/*  FUNCTION
	Change last access and last modification time of the given file to
	times specified in given utimbuf structure. If buf is NULL, the
	current time will be used instead.

	The utimbuf structure contains of two fields:

	time_t actime;  - last access time
	time_t modtime; - last modification time

    INPUTS
	filename - Name of the file
	buf - Pointer to utimbuf structure describing specified time.

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES
	This function can be used to set access and modification times with
	a resolution of 1 second, use utimes() if you need better precision.

    EXAMPLE

    BUGS
	Since AROS has no notion of last access time, actime field is silently
	ignored, only modification time of the file is set.

    SEE ALSO
	utimes()

    INTERNALS

******************************************************************************/
{
    struct timeval ts[2];
    
    if( buf == NULL )
    {
        time_t tt = time( NULL );

        ts[0].tv_sec = tt;
        ts[1].tv_sec = tt;
    }
    else
    {
        ts[0].tv_sec = buf->actime;
        ts[1].tv_sec = buf->modtime;
    }

    ts[0].tv_usec = 0;
    ts[1].tv_usec = 0;
    
    return utimes(filename, ts);
}
