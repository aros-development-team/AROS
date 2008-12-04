/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execl().
*/

#include <aros/debug.h>
#include <errno.h>
#include <stdlib.h>

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
    va_list args;
    char *default_argv[] = { NULL };
    char **argv;

    if(arg != NULL)
    {
        int argc = 1;
        va_start(args,arg);
        while(va_arg(args,const char *) != NULL)
            argc++;
        va_end(args);
        
        argv = (char**) malloc(sizeof(char*) * (argc + 1));
        if(!argv)
        {
            errno = ENOMEM;
            return -1;
        }
        
        argv[0] = (char*) arg;
        int argi;
        va_start(args,arg);
        for(argi = 1; argi < argc; argi++)
            argv[argi] = va_arg(args,char *);
        va_end(args);
        argv[argc] = NULL;
    }
    else
	argv = default_argv;
    
    return execve(path, argv, environ);
} /* execl() */

