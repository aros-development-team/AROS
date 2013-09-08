/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    SAS C function strrev().
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strrev (

/*  SYNOPSIS */
	char	   * s)

/*  FUNCTION
	Reverse a string (rotate it about its midpoint)

    INPUTS
	s - The string to be reversed

    RESULT
	The original string pointer

    NOTES
	SAS C specific

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello);
        strrev(buffer);
	
	// buffer now contains "olleH"


    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char *start, *end, c1, c2;

    start = end = s;
    
    while(*end) end++;
    end--;
    
    while(end > start)
    {
        c1 = *start;
	c2 = *end;
	
	*start++ = c2;
	*end--   = c1;
    }

    return s;
    
} /* strrev */
