/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wcstok(
    
/*  SYNOPSIS */    
    wchar_t * restrict s1, 
    const wchar_t * restrict s2, 
    wchar_t ** restrict ptr)

/*  FUNCTION
        Split wide string into tokens.

    INPUTS
        s1  - wide string to truncate.
        s2  - wide string containing the delimiter wide characters.
        ptr - Pointer to a wchar_t pointer.

    RESULT
        Returns a pointer to the first wide character of a token, 
        or a null pointer if there is no token. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{ 
	wchar_t *wcsBegin, *wcsEnd;

	if (s1 == NULL && ptr == NULL)
		return NULL;

	wcsBegin  = s1 ? s1 : *ptr;
	if (wcsBegin == NULL)
		return NULL;

	wcsBegin += wcsspn(wcsBegin, s2);
	if (*wcsBegin == '\0') {
		if (ptr)
			*ptr = NULL;
		return NULL;
	}

	wcsEnd = wcspbrk(wcsBegin, s2);
	if (wcsEnd && *wcsEnd != '\0')
		*wcsEnd++ = '\0';
	if (ptr)
		*ptr= wcsEnd;

	return wcsBegin;
}
