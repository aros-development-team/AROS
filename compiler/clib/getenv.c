/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function getenv()
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "__env.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

	char *getenv (

/*  SYNOPSIS */
	const char *name)

/*  FUNCTION
	Get an environment variable.

    INPUTS
	name - Name of the environment variable.

    RESULT
	Pointer to the variable's value, or NULL on failure.

    NOTES
        This function must not be used in a shared library.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	Based on libnix getenv

    HISTORY
	04.05.2001 stegerg created

******************************************************************************/
{
    __env_item *var = NULL;
    char  c;

    /*
      This will always return 0 if the var exists and EOF if it doesn't,
      then we'll be able to retrieve the var lenght with IoErr()
    */
    if (!GetVar((char *)name, &c, 1, GVF_BINARY_VAR))
    {
    	LONG len = IoErr();

	var = __env_getvar(name, len);

	if (var)
	{
	    /*This should not fail, unless someone stealt our variable*/
#warning FIXME: maybe this function should be atomic
    	    GetVar((char *)name, var->value, len, GVF_BINARY_VAR);
	}
    }

    return (var?var->value:NULL);
} /* getenv */

