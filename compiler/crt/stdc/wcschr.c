/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wcschr(
    
/*  SYNOPSIS */    
    const wchar_t* s, 
    wchar_t c)

/*  FUNCTION
        Find the first occurrence of c in the wide string s.
    
    INPUTS
        s - source wide string.
        c - wide character to locate.
    
    RESULT
        Returns a pointer to the located wide character c. 
        If c does not occur in the wide string s a null pointer is returned. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	while (1) {
		if (*s == c)
			return (wchar_t*)s;
		if (*s++ == L'\0')
			break;
	}

	return NULL;
}
