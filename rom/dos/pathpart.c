/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/10/21 17:43:11  aros
    A new function

    Desc:
    Lang: english
*/
#ifndef TEST
#include "dos_intern.h"
#else
#define __AROS_LH1(t,fn,a1,bt,bn,o,lib)     t fn (a1)
#define __AROS_LHA(t,n,r)                   t n
#define __AROS_FUNC_INIT
#define __AROS_BASE_EXT_DECL(bt,bn)
#define __AROS_FUNC_EXIT
#define CLIB_DOS_PROTOS_H
#include <exec/types.h>
#endif

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(STRPTR, PathPart,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, path, D1),

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    char * ptr;

    ptr = path;

    if (*ptr)
    {
	while (*ptr)
	{
	    if (*ptr == '/')
		path=ptr;
	    else if (*ptr == ':')
		path=ptr+1;

	    ptr ++;
	}
    }

    return path;
    __AROS_FUNC_EXIT
} /* PathPart */

#ifdef TEST

#include <stdio.h>

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

