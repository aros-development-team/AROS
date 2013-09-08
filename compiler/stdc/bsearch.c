/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function bsearch().
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void * bsearch (


/*  SYNOPSIS */
	const void * key,
	const void * base,
	size_t	     count,
	size_t	     size,
	int	  (* comparefunction)(const void *, const void *))

/*  FUNCTION
	Search in a sorted array for an entry key.

    INPUTS
	key - Look for this key.
	base - This is the address of the first element in the array
		to be searched. Note that the array *must* be sorted.
	count - The number of elements in the array
	size - The size of one element
	comparefunction - The function which is called when two elements
		must be compared. The function gets the addresses of two
		elements of the array and must return 0 is both are equal,
		< 0 if the first element is less than the second and > 0
		otherwise.

    RESULT
	A pointer to the element which equals key in the array or NULL if
	no such element could be found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char * base2 = (char *)base;
    size_t a	 = 0;
    size_t b	 = count;
    size_t c;
    int    d;

    /* Any elements to search ? */
    if (count != 0)
    {
	for (;;)
	{
	    /* Find the middle element between a and b */
	    c = (a + b) / 2;

	    /* Look if key is equal to this element */
	    if ((d = (*comparefunction)(key, &base2[size * c])) == 0)
		return &base2[size * c];

	    /*
		If the middle element equals the lower seach bounds, then
		there are no more elements in the array which could be
		searched (c wouldn't change anymore).
	    */
	    if (c == a)
		break;

	    /*
		The middle element is not equal to the key. Is it smaller
		or larger than the key ? If it's smaller, then c is our
		new lower bounds, otherwise c is our new upper bounds.
	    */
	    if (d < 0)
		b = c;
	    else
		a = c;
	}
    }

    /* Nothing found */
    return NULL;
} /* bsearch */
