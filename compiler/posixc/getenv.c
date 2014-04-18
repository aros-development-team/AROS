/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    C99 function getenv().
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
        The returned contents of the environment variable is cached per
        PosixCBase and per variable name. So the returned value is valid
        and does not change until a next call to getenv with the same
        PosixCBase and the same name.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	Based on libnix getenv

******************************************************************************/
{
    __env_item *var = NULL;
    char  c;

    /*
      This will always return 0 if the var exists and EOF if it doesn't,
      then we'll be able to retrieve the var length with IoErr()
    */
    if (!GetVar((char *)name, &c, 1, GVF_BINARY_VAR))
    {
    	LONG len = IoErr();

	var = __env_getvar(name, len+1); /* size == len + null-byte. */

	if (var)
	{
	    /* This should not fail, unless someone stole our variable */
            /* FIXME: maybe this function should be atomic? */
    	    GetVar((char *)name, var->value, len+1, GVF_BINARY_VAR);
	}
    }

    return (var?var->value:NULL);
} /* getenv */

