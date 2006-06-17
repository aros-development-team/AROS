/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Return a pointer to after the directories in a path.
    Lang: English
*/
#ifndef TEST
#    include "dos_intern.h"
#else
#    define AROS_LH1(t,fn,a1,bt,bn,o,lib)     t fn (a1)
#    define AROS_LHA(t,n,r)                   t n
#    define AROS_LIBFUNC_INIT
#    define AROS_LIBBASE_EXT_DECL(bt,bn)
#    define AROS_LIBFUNC_EXIT
#    define CLIB_DOS_PROTOS_H
#    include <exec/types.h>
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, PathPart,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, path, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 146, Dos)

/*  FUNCTION
	Returns a pointer to the character after the last
	directory in path (see examples).

    INPUTS
	path - Search this path.

    RESULT
	A pointer to a character in path.

    NOTES

    EXAMPLE
	PathPart("xxx:yyy/zzz/qqq") would return a pointer to the last '/'.
	PathPart("xxx:yyy") would return a pointer to the first 'y').

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    const char *ptr;

    /* '/' at the begining of the string really is part of the path */
    while (*path == '/')
    {
	++path;
    }

    ptr = path;

    while (*ptr)
    {
	if (*ptr == '/')
	{
	    path = ptr;
	}
	else if (*ptr == ':')
	{
	    path = ptr + 1;
	}
	
	ptr++;
    }
    
    return path;
    AROS_LIBFUNC_EXIT
} /* PathPart */

#ifdef TEST

#    include <stdio.h>

int main (int argc, char ** argv)
{
    UWORD i;
    STRPTR s,fileptr;

    while (--argc)
    {
	s = *++argv;
	fileptr = PathPart(s);

	printf("Pfad:  %s\nErg.: %s\n", s, fileptr);
    }
}

#endif /* TEST */

