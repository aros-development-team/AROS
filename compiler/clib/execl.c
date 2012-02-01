/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execl().
*/

#include <aros/debug.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "__arosc_privdata.h"
#include "__exec.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int execl(

/*  SYNOPSIS */
	const char *path, 
	const char *arg, ...)
        
/*  FUNCTION
	Executes a file located in given path with specified arguments.

    INPUTS
	path - Pathname of the file to execute.
	arg - First argument passed to the executed file.
	... - Other arguments passed to the executed file.

    RESULT
	Returns -1 and sets errno appropriately in case of error, otherwise
	doesn't return.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	execve(), execlp(), execv(), execvp()
	
    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();

    va_list args;
    char *const *argv;
    char **environ = (aroscbase->acb_environptr) ? *aroscbase->acb_environptr : NULL;

    va_start(args, arg);
    
    if(!(argv = __exec_valist2array(arg, args)))
    {
        errno = ENOMEM;
        return -1;
    }

    va_end(args);

    APTR id = __exec_prepare(path, 0, argv, environ);
    __exec_cleanup_array();
    if (!id)
        return -1;

    __exec_do(id);
    
    assert(0); /* Should not be reached */
    return -1;
} /* execl() */

