/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

wchar_t *wcsncat(

/*  SYNOPSIS */
    wchar_t * restrict s1, 
    const wchar_t * restrict s2, 
    size_t n)

/*  FUNCTION
         Appends not more than n wide characters from the array pointed to by s2 to the end of the wide string pointed to by s1.

    INPUTS
        s1 - specifies the pointer to the destination array.
        s2 - specifies the string to be added to the destination.
        n  - specifies the maximum number of wide characters to be added.

    RESULT
        Returns the value of s1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	wchar_t* dest = s1;
	const wchar_t* srcEnd = s2 + n;

	while (*dest != L'\0')
		dest++;
	while (s2 < srcEnd && *s2 != L'\0')
		*dest++ = *s2++;
	*dest = L'\0';

	return s1;
}