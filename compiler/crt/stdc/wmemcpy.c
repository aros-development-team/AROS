/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wmemcpy().
*/

#include <wchar.h>
#include <string.h>

/*****************************************************************************

    NAME */

wchar_t *wmemcpy(

/*  SYNOPSIS */  
    wchar_t * restrict s1, 
    const wchar_t * restrict s2, 
    size_t n)

/*  FUNCTION
         Copies n wide characters from the object pointed to by s2
         to the object pointed to by s1. 

    INPUTS
        s1 - Pointer to the destination array where the content is to be copied.
        s2 - Pointer to the source of data to be copied.
        n  - Number of bytes to copy.

    RESULT
        Returns the value of s1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	return memcpy(s1, s1, n * sizeof(wchar_t));
}
