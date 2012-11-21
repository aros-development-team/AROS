/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Split a path into pieces
    Lang: english
*/
#ifndef TEST
#    include "dos_intern.h"
#else
#    include <proto/dos.h>
#    undef SplitName
#    undef AROS_LH5
#    define AROS_LH5(t,fn,a1,a2,a3,a4,a5,bt,bn,o,lib)     t fn (a1,a2,a3,a4,a5)
#    undef AROS_LHA
#    define AROS_LHA(t,n,r)                   t n
#    undef AROS_LIBFUNC_INIT
#    define AROS_LIBFUNC_INIT
#    undef AROS_LIBFUNC_EXIT
#    define AROS_LIBFUNC_EXIT
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH5(LONG, SplitName,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(ULONG , separator, D2),
        AROS_LHA(STRPTR, buf, D3),
        AROS_LHA(LONG  , oldpos, D4),
        AROS_LHA(LONG  , size, D5),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 69, Dos)

/*  FUNCTION
        Split a path into parts at the position of separator.

    INPUTS
        name - Split this path
        separator - Split it at this separator
        buf - Copy the current part into this buffer
        oldpos - Begin at this place with the search for separator.
                If you call this function for the first time, set it
                to 0.
        size - The size of the buffer. If the current part of the
                path is bigger than size-1, only size-1 bytes will
                be copied.

    RESULT
        The next position to continue for the next part or -1 if
        there is no separator after name+oldpos.

    NOTES

    EXAMPLE
        See below.

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    size --;

    name += oldpos;

    while (*name != separator && *name && size)
    {
        size --;
        *buf++ = *name++;
        oldpos ++;
    }

    *buf = 0;

    if (*name == separator)
        return oldpos + 1;

    return -1;
    AROS_LIBFUNC_EXIT
} /* SplitName */

#ifdef TEST

#    include <stdio.h>
#    include <dos/dos.h>

#    include <proto/dos.h>

int main (int argc, char ** argv)
{
    LONG pos;
    UBYTE buffer[256];

    if (argc < 3)
    {
        fprintf (stderr, "Usage: %s <path> <separator>\n", argv[0]);
        return RETURN_ERROR;
    }

    pos=0;

    do
    {
        pos = SplitName (argv[1], *(argv[2]), buffer, pos, sizeof(buffer));

        printf ("pos = %3ld  buffer = \"%s\"\n", pos, buffer);
    }
    while (pos != -1);

    return RETURN_OK;
}

#endif /* TEST */
