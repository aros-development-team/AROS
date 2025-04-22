/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

size_t wcsspn(
    
/*  SYNOPSIS */
    const wchar_t *s1, 
    const wchar_t *s2)

/*  FUNCTION
        Calculates the length of the initial segment of s1 which consists
        entirely of wide characters in s2.

    INPUTS
        s1 - wide string to be scanned.
        s2 - wide string containing the characters to match.

    RESULT
        Length of the initial segment of s1 which contains only
        wide characters from s2.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	const wchar_t *wcPointer = s1;
	wchar_t wc;
	for (; (wc = *wcPointer) != L'\0'; ++wcPointer) {
		const wchar_t *accept;
		for (accept = s2; *accept != L'\0'; ++accept) {
			if (*accept == wc)
				break;
		}
		if (*accept == L'\0')
			break;
	}

	return wcPointer - s1;
}
