/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include <exec/types.h>
#include <assert.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	char *mktemp (

/*  SYNOPSIS */
	char *template)

/*  FUNCTION
	Make a unique temporary file name.

    INPUTS
	template - template to change into unique filename

    RESULT
	Returns template.

    NOTES
    	Template must end in "XXXXXX" (i.e at least 6 X's).
    	
        Prior to this paragraph being created, mktemp() sometimes produced filenames
        with '/' in them. AROS doesn't like that at all. Fortunately, the bug in this
        function which produced it has been fixed. -- blippy
		
        For clarity, define the HEAD of the template to be the part before the tail,
        and the TAIL to be the succession of X's. So in, T:temp.XXXXXX , the head is
        T:temp. and the tail is XXXXXX .
        
    EXAMPLE

    BUGS
    	Cannot create more than 26 filenames for the same process id. This is because
    	the "bumping" is only done to the first tail character - it should be
    	generalised to bump more characters if necessary.

    SEE ALSO

    INTERNALS
    	Based on libnix mktemp

******************************************************************************/
{ 
    IPTR pid = (IPTR)FindTask(0L);
    char *c = template + strlen(template);
    BPTR  lock;
    IPTR remainder;
    
    while (*--c == 'X')
    {
        remainder = pid % 10;
        assert(remainder>=0 && remainder<10); 
        *c = remainder + '0';
        pid /= 10L;
    }
    
    c++; /* ... c now points to the 1st char of the template tail */

    /* If template errornously does not end in X c will point to '\0';
       exit gracefully
    */
    if (*c)
    {
        /* Loop over the first position of the tail, bumping it up as necessary */
        for(*c = 'A'; *c <= 'Z'; (*c)++)
        {
            if (!(lock = Lock(template, SHARED_LOCK)))
            {
                if (IoErr() != ERROR_OBJECT_IN_USE)
                {
                    D(bug("No lock (IoErr: %d); returning '%s'\n", IoErr(), template));
                    return template;
                }
            }
            UnLock(lock);
        }
    }
    
    D(bug("26 tries exhausted; Returning '%s'\n", template));
    return template; 
} /* mktemp */
