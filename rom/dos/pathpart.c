/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1997/01/27 00:36:27  ldp
    Polish

    Revision 1.4  1996/12/09 13:53:37  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.3  1996/11/21 10:49:47  aros
    Created macros AROS_SLIB_ENTRY() for assembler files, too, to solve naming
    problems.

The #includes
    makedepend will ignore them (GCC works, though).

    Removed a couple of Logs

    Revision 1.2  1996/10/24 15:50:34  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:43:11  aros
    A new function

    Desc:
    Lang: english
*/
#ifndef TEST
#include "dos_intern.h"
#else
#define AROS_LH1(t,fn,a1,bt,bn,o,lib)     t fn (a1)
#define AROS_LHA(t,n,r)                   t n
#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(bt,bn)
#define AROS_LIBFUNC_EXIT
#define CLIB_DOS_PROTOS_H
#include <exec/types.h>
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, PathPart,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, path, D1),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
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
    AROS_LIBFUNC_EXIT
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

