/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar.h>

/*****************************************************************************

    NAME */

int wmemcmp(
    
/*  SYNOPSIS */     
    const wchar_t * s1, 
    const wchar_t * s2, 
    size_t n)

/*  FUNCTION
         Compares the first n wide characters of the object pointed to by s1 
         to the first n wide characters of the object pointed to by s2.

    INPUTS
        s1 - Pointer to block of elements of type wchar_t.
        s2 - Pointer to block of elements of type wchar_t.
        n  - Number of elements of type wchar_t to compare.

    RESULT
        Returns an integral value indicating the relationship between the content of the blocks:
        A zero value indicates that the contents of both memory blocks are equal.
        A value greater than zero indicates that the first wide character that does not match 
        in both memory blocks has a greater value in ptr1 than in ptr2; 
        And a value less than zero indicates the opposite.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
	while (n-- > 0) {
		const wchar_t wc1 = *s1++;
		const wchar_t wc2 = *s2++;
		if (wc1 > wc2)
			return 1;
		else if (wc2 > wc1)
			return -1;
	}

	return 0;
}