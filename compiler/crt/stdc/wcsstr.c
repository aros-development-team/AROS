/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

wchar_t *wcsstr(
    
/*  SYNOPSIS */        
    const wchar_t *s1, 
    const wchar_t *s2)

/*  FUNCTION
        Locates the first occurrence in the wide string pointed to by s1 of the sequence 
        of wide characters (excluding the terminating null wide character) in the wide string 
        pointed to by s2.

    INPUTS
        s1  - wide string to be scanned.
        s2  - wide string containing the sequence of characters to match.

    RESULT
        Returns a pointer to the first occurrence in s1 of the entire sequence of characters 
        specified in s2, or a null pointer if the sequence is not present in s1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	if (*s2 == L'\0')
		return (wchar_t*)s1;

	for (; *s1 != L'\0'; ++s1) {
		const wchar_t *needle = s2;
		const wchar_t *haystackPointer = s1;
		while (*needle == *haystackPointer++ && *needle != 0)
			++needle;
		if (*needle == L'\0')
			return (wchar_t*)s1;
	}

	return NULL;
}
