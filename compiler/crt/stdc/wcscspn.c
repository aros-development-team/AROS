/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

size_t wcscspn(
    
/*  SYNOPSIS */    
    const wchar_t *s1, 
    const wchar_t *s2)

/*  FUNCTION
        Scans s1 for the first occurrence of any of the wide characters that are part of s2, 
        returning the number of wide characters of s1 read before this first occurrence..

    INPUTS
        s1  - wide string to be scanned.
        s2  - wide string containing the characters to match.

    RESULT
        Returns the length of the segment.

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
		const wchar_t *reject;
		for (reject = s2; *reject != L'\0'; ++reject) {
			if (*reject == wc)
				return wcPointer - s1;
		}
	}

	return wcPointer - s1;
}
