/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function getenv().
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <errno.h>

#include "__stdcio_intbase.h"

#define DEBUG 0
#include <aros/debug.h>

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
        When no memory is available errno will be set to ENOMEM.

    NOTES
        The returned contents of the environment variable is cached per
        StdCIOBase. So the returned value is valid and does not change
        until a next call to getenv on the same StdCIOBase.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	Based on libnix getenv

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase;
    char  c;

    /*
      This will always return 0 if the var exists and EOF if it doesn't,
      then we'll be able to retrieve the var lenght with IoErr()
    */
    if (!GetVar((char *)name, &c, 1, GVF_BINARY_VAR))
    {
    	LONG len = IoErr();

        D(bug("[getenv] Variable found of size %d\n", size));

        if (len + 1 > StdCIOBase->varsize)
        {
            if (StdCIOBase->envvar)
                FreeMem(StdCIOBase->envvar, StdCIOBase->varsize);

            StdCIOBase->envvar = AllocMem(len + 1, MEMF_ANY);
            if (StdCIOBase->envvar == NULL)
            {
                StdCIOBase->varsize = 0;
                errno = ENOMEM;
                return NULL;
            }
            StdCIOBase->varsize = len + 1;
	}

        /* This should not fail, unless someone stealt our variable*/
        /* FIXME: maybe this function should be atomic */
        GetVar((char *)name, StdCIOBase->envvar, StdCIOBase->varsize, GVF_BINARY_VAR);

        D(bug("[getenv] Got value \"%s\"\n", StdCIOBase->envvar));

        return (StdCIOBase->envvar);
    }
    else
    {
        D(bug("[getenv] Variable not found\n"));

        return NULL;
    }
} /* getenv */

