/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: unix_funcs.c 12745 2001-12-08 19:36:48Z chodorowski $
*/

/* Some POSIX includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>


/*
 * Some wrapper functions for the unix functions.
 * Some of the constants have to be brought in here
 * because they clash with AROS constants otherwise.
 */
int unix_open_nonblock(const char * pathname)
{
	return open(pathname, O_NONBLOCK|O_RDWR);
}
