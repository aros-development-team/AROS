/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function getenv()
    Lang: english
*/

#include <proto/dos.h>

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
    static char *var = NULL;
    size_t  	len, i = 0;
    
    do
    {
    	i += 256;
	if (var != NULL) free(var);
	    
	var = malloc(i);
	if (var == NULL) return NULL;
	   
	len = GetVar((char *)name, var, i, GVF_BINARY_VAR) + 1;
	
    } while(len >= i);
    
    if (len == 0)
    {
    	/* Variable does not exist */
    	return NULL;
    }
    
    return var;
    
} /* getenv */

