/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	char *mktemp (

/*  SYNOPSIS */
	char *template)

/*  FUNCTION
	Make a unique temporary file name.

    INPUTS
	template- Name of the environment variable.

    RESULT
	Returns template.

    NOTES
    	Template must end in "XXXXXX".

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	Based on libnix mktemp

******************************************************************************/
{ 
    AROS_GET_SYSBASE_OK
    long  pid = (long)FindTask(0L);
    char *c = template + strlen(template);
    BPTR  lock;
    
    while (*--c == 'X')
    {
	*c = pid % 10 + '0';
	pid /= 10;
    }
    
    c++;
    
    if (*c)
    {
	for(*c = 'A'; *c <= 'Z'; (*c)++)
	{
	    if (!(lock = Lock(template, ACCESS_READ)))
	    {
        	return template;
	    }
	    UnLock(lock);
	}
	*c = 0;
    }
    
    return template; 
 
} /* mktemp */

