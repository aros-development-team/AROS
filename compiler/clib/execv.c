/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execv().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int execv(

/*  SYNOPSIS */
	const char *path, 
        char *const argv[])
        
/*  FUNCTION
	Executes a file with specified arguments.

    INPUTS
        file - Name of the file to execute.
        argv - Array of arguments given to main() function of the executed
        file.

    RESULT
        0 in case of success. In case of failure errno is set appropriately
        and function returns -1.

    NOTES

    EXAMPLE

    BUGS
        See execve documentation.

    SEE ALSO
        execve
	
    INTERNALS

******************************************************************************/
{
    return execve(path, argv, environ);
} /* execv() */
