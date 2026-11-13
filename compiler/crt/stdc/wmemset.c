/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>
#include <string.h>

/*****************************************************************************

    NAME */

wchar_t *wmemset(

/*  SYNOPSIS */  
    wchar_t *s, 
    wchar_t c, 
    size_t n)

/*  FUNCTION
        Copies the value of c into each of the first n wide characters of the object pointed to by s.

    INPUTS
        s - Pointer to the array to fill.
        c - Value to be set.
        n - Number of bytes to be set to the value.

    RESULT
        Returns the value of s. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    wchar_t *dest = s;
	while (n-- > 0)
		*dest++ = c;

	return s;
}
