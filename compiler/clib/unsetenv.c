/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function unsetenv().
*/

#include <proto/dos.h>
#include "__env.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void unsetenv (

/*  SYNOPSIS */
	const char *name)

/*  FUNCTION
	 deletes a variable from the environment.

    INPUTS
	name  --  Name of the environment variable to delete.

    RESULT
       Returns zero on success, or -1 if the variable was not found.

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{

    __env_delvar(name);
    DeleteVar(name, GVF_LOCAL_ONLY);
} /* unsetenv */

