/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strcoll().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	size_t strxfrm (

/*  SYNOPSIS */
        char * restrict dst,
	const char * restrict src,
	size_t n)

/*  FUNCTION
        The strxfrm() function transforms a null-terminated string pointed to by
        src according to the current locale collation if any, then copies the
        transformed string into dst.  Not more than n characters are copied into
        dst, including the terminating null character added.  If n is set to 0
        (it helps to determine an actual size needed for transformation), dst is
        permitted to be a NULL pointer.

        Comparing two strings using strcmp() after strxfrm() is equal to compar-
        ing two original strings with strcoll().

    INPUTS
	dst - the destination string's buffer
	src - the source string
	n   - the size of the dst buffer.

    RESULT
        Upon successful completion, strxfrm() returns the length of the trans-
        formed string not including the terminating null character.  If this
        value is n or more, the contents of dst are indeterminate.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    #warning implement strxfrm() properly
    size_t srclen = strlen(src);

    strncpy(dst, src, n);

    return srclen;
} /* strxfrm */

