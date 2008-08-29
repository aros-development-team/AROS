/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execvp().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	int execvp(

/*  SYNOPSIS */
	const char *file, 
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
    char *path, *path_item, *path_ptr;
    char *emptyenv[] = { NULL };
    char default_path[] = ":/bin:/usr/bin";
    char *full_path;
    int saved_errno;
    
    if(index(file, '/') || index(file, ':'))
    {
	/* file argument is a path, don't search */
	return execve(file, argv, emptyenv);
    }
    
    /* argument is a file, search for this file in PATH variable entries */
    
    if(!(path = getenv("PATH")))
	path = default_path;
    else
	path = strdup(path);
    
    path_ptr = path;
    
    while((path_item = strsep(&path_ptr, ",:"))) 
    {
	if(path_item[0] == '\0')
	    path_item = ".";
	
	if(full_path = malloc(strlen(path_item) + strlen(file) + 2))
	{
	    full_path[0] = '\0';
	    strcat(full_path, path_item);
	    strcat(full_path, "/");
	    strcat(full_path, file);
	    
	    /* try executing execve with this path */
	    if(execve(full_path, argv, emptyenv) == 0)
	    {
		free(full_path);
		return 0;
	    }
	    else
		if(errno == EACCES)
		    saved_errno = EACCES;
	    free(full_path);
	}
	else
	{
	    saved_errno = ENOMEM;
	    goto error;
	}
    }

    /* set ENOENT error if there were errors other than EACCES */
    if(saved_errno != EACCES)
	saved_errno = ENOENT;

error:
    errno = saved_errno;
    return -1;
} /* execvp() */

