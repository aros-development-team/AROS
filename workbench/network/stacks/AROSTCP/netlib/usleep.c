/* $Id$
 *
 *      usleep.c - suspend process for the specified time
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/param.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

/****** net.lib/usleep *********************************************

    NAME
	usleep - suspend process execution for the specified time

    SYNOPSIS
	void usleep(unsigned int microseconds);

    FUNCTION
        Process execution is suspended for number of microseconds
        specified in 'microseconds'. The sleep will be aborted if any
        of the break signals specified for the process is received
        (only CTRL-C by default).

    PORTABILITY
	UNIX

    INPUTS
	'microseconds' - number of microseconds to sleep.

    RESULT
        Does not return a value.

    NOTES
        The sleep is implemented as a single select() call with all other
        than time out argument as NULL.

    SEE ALSO
	bsdsocket.library/select()

*****************************************************************************
*
*/

void usleep(unsigned int usecs)
{
  struct timeval tv;

  tv.tv_sec = 0;
  while (usecs >= 1000000) {
    usecs -= 1000000;
    tv.tv_sec++;
  }	
  tv.tv_usec = usecs;
  select(0, 0, 0, 0, &tv);
}
