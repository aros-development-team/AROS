/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

wchar_t *wcspbrk(
    
/*  SYNOPSIS */
    const wchar_t *s1, 
    const wchar_t *s2)

/*  FUNCTION
         Locate characters s2 in wide string s1.

    INPUTS
        s1 - wide string to be scanned.
        s2 - wide string containing the characters to match.

    RESULT
        Returns a pointer  to the wide character in s1, or a null pointer if no wide character from s2 occurs in s1. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	for (; *s1 != L'\0'; ++s1) {
		const wchar_t* accept = s2;
		for (; *accept != L'\0'; ++accept) {
			if (*accept == *s1)
				return (wchar_t*)s1;
		}
	}

	return NULL;
}
