/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX/C23 function memccpy().
*/

/*****************************************************************************

    NAME */
#include <string.h>

        void * memccpy (

/*  SYNOPSIS */
        void       * restrict dest,
        const void * restrict src,
        int          c,
        size_t       n)

/*  FUNCTION
        Copy bytes from src to dest, stopping after the first occurrence of
        the byte c (converted to unsigned char) has been copied, or after n
        bytes have been copied, whichever comes first.

    INPUTS
        dest - destination buffer.
        src  - source buffer.
        c    - stop copying once this byte (as unsigned char) is copied.
        n    - maximum number of bytes to copy.

    RESULT
        A pointer to the byte in dest immediately following the copy of c, or
        NULL if c was not found in the first n bytes of src.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        memcpy(), memmove(), strcpy()

    INTERNALS

******************************************************************************/
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    unsigned char        uc = (unsigned char)c;

    while (n--)
    {
        if ((*d++ = *s++) == uc)
            return d;
    }

    return NULL;
} /* memccpy */
