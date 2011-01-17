/* $Id$
 *
 *      utime.c - set the modification date of the file
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utime.h>
#include <errno.h>

#include <syslog.h>

#include "netlib.h"

/****** net.lib/utime *********************************************

    NAME
	utime - set file access and modification times

    SYNOPSIS
	#include <utime.h>

	int error = utime(const char *name, const struct utimbuf *times)

    FUNCTION
	The access and modification times for the file 'name' are modified
	according to the 'times'. If 'times' is NULL, the times are set to
	systems current time.

    PORTABILITY
	UNIX

    INPUTS
	'name'  - the name of the file to be affected.

	'times' - pointer to a structure containing the time values,
		  defined in <utime.h> as:

		      struct utimbuf {
			  time_t actime;	\* Access time *\
			  time_t modtime;	\* Modification time *\
		      };

		  Both times are in units of seconds since Jan. 1, 1970,
		  Greenwich Mean Time.

		  If the 'times' is given as the NULL pointer, the current
		  time is used.

    RESULT
	Returns 0 when successful and -1 with specific error code in errno in
	case of an error.

    NOTES
	Since AmigaDOS files have only one time stamp, both access and
	modification times cannot be supported. Since the AmigaDOS file date
	is the modification time, only the 'modtime' field of the 'times' is
	used.

	The conversion from 1.1.1970 based GMT to 1.1.1978 based local time is
	done with external long __local_to_GMT, which is defined and
	initialized by the timerinit.c module included in the net.lib.

    SEE ALSO
	dos.library/DateStamp(), dos.library/SetFileDate(), net.lib/timerinit

*****************************************************************************
*
*/

extern long __local_to_GMT; /* defined and initialized in timerinit.c */

int
utime(const char *name, const struct utimbuf *times)
{
  struct DateStamp stamp;
  unsigned long days, secs;
  time_t time;

  if (times == NULL)
    DateStamp(&stamp);
  else {
    /*
     * AmigaDOS file date is the modification time
     */
    time = times->modtime;

    /*
     * Convert time (secs since 1.1.1970 GMT) to
     * AmigaDOS DateStamp (based on 1.1.1978 local time).
     */
    time -= __local_to_GMT; /* GMT to local */
    days = (unsigned long)time / (unsigned long)(24*60*60);
    secs = (unsigned long)time % (unsigned long)(24*60*60);
    stamp.ds_Days = (LONG)days;
    stamp.ds_Minute = (LONG)(secs / 60);
    stamp.ds_Tick = (LONG)((secs % 60) * TICKS_PER_SECOND);
  }

  if (!SetFileDate((STRPTR)name, &stamp)) {
    set_errno(IoErr());
    return -1;
  }

  return 0;
}
