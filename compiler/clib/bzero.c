/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: bzero()
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
	#include <string.h>

	void bzero (

/*  SYNOPSIS */
	void *       ptr,
	size_t       len)

/*  FUNCTION
	Write len zero bytes to ptr. If len is zero, does nothing.

    INPUTS
	ptr - The first byte of the area in memory to be cleared.
	len - How many bytes to clear.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	28-10-96    ldp created

******************************************************************************/
{
    char * ptr2 = ptr;

    while(len--)
	*ptr2++ = 0;

} /* bzero */

