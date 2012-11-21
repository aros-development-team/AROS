/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Returns a pointer to the first char of the filename in the give file part.
*/
#ifndef TEST
#    include "dos_intern.h"
#else
#    define AROS_LH1(t,fn,a1,bt,bn,o,lib)     t fn (a1)
#    define AROS_LHA(t,n,r)                   t n
#    define AROS_LIBFUNC_INIT
#    define AROS_LIBFUNC_EXIT
#    include <exec/types.h>
#    define CLIB_DOS_PROTOS_H
#endif
#include <string.h>


/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(STRPTR, FilePart,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, path, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 145, Dos)

/*  FUNCTION
        Get a pointer to the last component of a path, which is normally the
        filename.

    INPUTS
        path - pointer AmigaDOS path string
            May be relative to the current directory or the current disk.

    RESULT
        A pointer to the first char of the filename!

    NOTES

    EXAMPLE
        FilePart("xxx:yyy/zzz/qqq") returns a pointer to the first 'q'.
        FilePart("xxx:yyy")         returns a pointer to the first 'y'.
        FilePart("yyy")             returns a pointer to the first 'y'.

    BUGS
        None known.

    SEE ALSO
        PathPart(), AddPart()

    INTERNALS
        Goes from the last char of the pathname back until it finds a ':',
        a '/' or until the first char reached.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(path)
    {
        CONST_STRPTR i;

        /* set i to last char of path */

        if (!*path) /* path == "" ? */
          return (STRPTR)path;

        i = path + strlen (path) -1;   /* set i to the \0-byte */

        /* decrease pointer as long as there is no ':', no '/' or till
          the first char anyway. hope this works in all situations */
        while ((*i != ':') && (*i != '/') && (i != path))
            i--;

        if ((*i == ':')) i++;
        if ((*i == '/')) i++;

        return (STRPTR)i;
    } /* path */

    return NULL;  /* if no path is given, return NULL pointer (shouldn't happen) */

    AROS_LIBFUNC_EXIT
} /* FilePart */

#ifdef TEST

#    include <stdio.h>

int main (int argc, char ** argv)
{
    UWORD i;
    CONST_STRPTR s, fileptr;

    while (--argc)
    {
        s = *++argv;
        fileptr = FilePart(s);

        printf("Pfad:  %s\nDatei: %s\n", s, fileptr);
    }
}

#endif /* TEST */
