/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
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
	do {
		if (*s == c)
			return (wchar_t*)s;
	} while (*s++);

	return NULL;
}
