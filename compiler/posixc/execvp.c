/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function execvp().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "__exec.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int execvp(

/*  SYNOPSIS */
	const char *file, 
	char *const argv[])
        
/*  FUNCTION
	Executes a file with given name. The search paths for the executed
	file are paths specified in the PATH environment variable.

    INPUTS
	file - Name of the file to execute.
	argv - Array of arguments given to main() function of the executed
	file.

    RESULT
	Returns -1 and sets errno appropriately in case of error, otherwise
	doesn't return.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	execve(), execl(), execlp(), execv()
	
    INTERNALS

******************************************************************************/
{
    char ***environptr = __posixc_get_environptr();
    char **environ = (environptr != NULL) ? *environptr : NULL;
    APTR id = __exec_prepare(file, 1, argv, environ);
    if(!id)
        return -1;
    
    __exec_do(id);
    
    assert(0); /* not reached */
    return -1;
} /* execvp() */

