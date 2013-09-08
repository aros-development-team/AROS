/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function system().
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int system (

/*  SYNOPSIS */
	const char *string)

/*  FUNCTION
        Execute a command string. If string is NULL then 1 will be returned.

    INPUTS
        string - command to execute or NULL

    RESULT
        Return value of command executed. If value < 0 errno indicates error.
        1 is return if string is NULL.

    NOTES
        The system() version of stdcio.library just passes the command
        to SystemTags() dos.library call.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int ret;

    if (string == NULL)
        return 1; /* We have AmigaShell */

    ret = (int)SystemTags(string, NULL);
    
    if (ret == -1)
        errno = __stdc_ioerr2errno(IoErr());

    return ret;
} /* system */

