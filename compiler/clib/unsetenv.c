/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function getenv()
    Lang: english
*/

#include <proto/dos.h>
#include "__env.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int unsetenv (

/*  SYNOPSIS */
	const char *name)

/*  FUNCTION
	 deletes a variable from the environment.

    INPUTS
	name - Name of the environment variable to delete.

    RESULT
       Returns zero on success, or -1 if the variable was not found.

    NOTES
        This implementation extends the BSD4.3 one in that it returns a value.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS


    HISTORY
	04.22.2001 falemagn created

******************************************************************************/
{

    __env_delvar(name);
    return -!DeleteVar(name, GVF_LOCAL_ONLY);
} /* unsetenv */

