/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/12/09 13:53:51  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.1  1996/12/06 15:19:40  aros
    Initial revision


    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH2(LONG, WriteChars,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, buf, D1),
	AROS_LHA(ULONG , buflen, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 157, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	5-12-96    turrican automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    ULONG i;
    BPTR file=((struct Process *)FindTask(NULL))->pr_COS;

    for(i=0;i<buflen;i++)
	if(FPutC(file,buf[i])<0)
	    return EOF;

    return (LONG)i;

    AROS_LIBFUNC_EXIT
} /* WriteChars */
