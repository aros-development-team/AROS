/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function memchr().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	void * memchr (

/*  SYNOPSIS */
	const void * mem,
	int	     c,
	size_t	     n)

/*  FUNCTION
        Locate the first occurence of c which is converted to an unsigned
        char in the first n bytes of the memory pointed to by mem.

    INPUTS
        mem - pointer to memory that is searched for c
          c - the character to search for
          n - how many bytes to search through starting at mem

    RESULT
        pointer to the located byte or null if c was not found

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    /* unsigned char to compare chars > 127 */
    const unsigned char * ptr = (unsigned char *)mem;

    while (n)
    {
        if (*ptr == (unsigned char)c)
	    return ((void *)ptr);

	n --;
	ptr ++;
    }

    return NULL;
} /* memchr */
