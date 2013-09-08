/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * stpcpy (

/*  SYNOPSIS */
	char	   * dest,
	const char * src)

/*  FUNCTION
	Copy a string returning pointer to its end.

    INPUTS
	dest - The string is copied into this variable. Make sure it is
	       large enough.
	       
	src - This is the new contents of dest.

    RESULT
	pointer to the end of the string dest (address of it's null
	character)

    NOTES
	No check is made that dest is large enough for src.

    EXAMPLE

    BUGS

    SEE ALSO
	strcpy()

    INTERNALS

******************************************************************************/
{
    char * ptr = dest;

    while ((*ptr = *src))
    {
	ptr ++;
	src ++;
    }

    return ptr;
    
} /* stpcpy */
