/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execvp().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int execvp(

/*  SYNOPSIS */
	const char *file, 
        char *const argv[])
        
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
#   warning Implement execvp()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    return -1;
} /* execvp() */

