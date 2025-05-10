/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

wchar_t *wcsrchr(

/*  SYNOPSIS */
    const wchar_t *s, 
    wchar_t c)

/*  FUNCTION
         Locates the last occurrence of c in the wide string pointed to by s.

    INPUTS
        s - wide string.
        c - wide character to be located.

    RESULT
        Returns a pointer to the last occurrence of c in s.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	const wchar_t* wcs = s + wcslen(s);
	for (; wcs >= s; --wcs) {
		if (*wcs == c)
			return (wchar_t*)wcs;
	}

	return NULL;
}