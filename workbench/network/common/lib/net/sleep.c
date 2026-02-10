/* $Id$
 *
 *      sleep.c - suspend process for the specified time
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/param.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

/****** net.lib/sleep *********************************************

    NAME
	sleep - suspend process execution for the specified time

    SYNOPSIS
	void sleep(unsigned int seconds);

    FUNCTION
        Process execution is suspended for number of seconds specified in 
        'seconds'. The sleep will be aborted if any of the break signals
        specified for the process is received (only CTRL-C by default).

    PORTABILITY
	UNIX

    INPUTS
	'seconds' - number of seconds to sleep.

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

void sleep(unsigned int secs)
{
  struct timeval tv;

  tv.tv_sec = secs;
  tv.tv_usec = 0;
  select(0, 0, 0, 0, &tv);
}
