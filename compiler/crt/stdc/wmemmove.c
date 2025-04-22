/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wmemmove().
*/

#include <wchar.h>
#include <string.h>

/*****************************************************************************

    NAME */

wchar_t *wmemmove(
    
/*  SYNOPSIS */  
    wchar_t *s1, 
    const wchar_t *s2, 
    size_t n)

/*  FUNCTION
         Copies n wide characters from the object pointed to by s2
         to the object pointed to by s1.
         Copying takes place as if an intermediate buffer were used, 
         allowing the destination and source to overlap 

    INPUTS
        s1 - Pointer to the destination array where the content is to be copied.
        s2 - Pointer to the source of data to be copied.
        n  - Number of elements of type wchar_t to copy.

    RESULT
        Returns the value of s1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	return memmove(s1, s1, n * sizeof(wchar_t));
}
