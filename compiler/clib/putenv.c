/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function putenv().
*/

#include <proto/dos.h>
#include <dos/var.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int putenv (

/*  SYNOPSIS */
	const char *string)

/*  FUNCTION
	Change or add an environment variable.

    INPUTS
	string - Is of the form "name=value", where name is the variable's
	         name and value is its value. In case the string is of the form
		 "name" then the variable is removed from the environment.
    RESULT
	The putenv() function returns zero on success, or -1 if an
       	error occurs. In such a case the errno variable is set
       	appropriately.

    NOTES
        This function must not be used in a shared library.
	Conforming to BSD4.4 in that it makes a copy of the argument string.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char *name = NULL, *value = NULL, *ptr;
    int res = -1;

    if (!string)
    {
    	errno = EFAULT;
	goto err;
    }

    name = strdup(string);
    if (!name) return -1;

    for (ptr=name; *ptr!='\0' && *ptr!='='; ptr++)
    {
	if (isspace(*ptr))
	{
	    errno = EINVAL;
	    goto err;
     	}
    }

    /* No value means we have to delete the variable */
    if (*ptr == '\0')
	res = 0, unsetenv(name);

    /* we might have a value to get */
    else
    {
    	*ptr = '\0'; /*terminate the name string */
	for (value=++ptr; *ptr!='\0'; ptr++)
    	{
	    if (isspace(*ptr))
	    {
	    	errno = EINVAL;
	    	goto err;
     	    }
    	}

	res = setenv(name, value, 1);
    }

err:
    if (name)
    	free(name);

    return res;
} /* putenv */

