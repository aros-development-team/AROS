/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

wchar_t *wmemchr(
    
/*  SYNOPSIS */  
    const wchar_t *s, 
    wchar_t c, 
    size_t n)

/*  FUNCTION
         Locates the first occurrence of c in the initial n wide characters of the object pointed to by s. .

    INPUTS
        s  - Pointer to the array of wchar_t elements to be searched.
        c  - Wide character to be located.
        n  - Number of elements of type wchar_t to compare.

    RESULT
        Returns a pointer to the located wide character, 
        or a null pointer if the wide character does not occur in the object.  

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	while (n-- > 0) {
		if (*s == c)
			return (wchar_t*)s;
		++s;
	}

	return NULL;
}