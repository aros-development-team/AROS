/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(LONG, FPuts,

/*  SYNOPSIS */
	__AROS_LHA(BPTR,   file,   D1),
	__AROS_LHA(STRPTR, string, D2),

/*  LOCATION */

	struct DosLibrary *, DOSBase, 56, Dos)

/*  FUNCTION

    INPUTS
	file   - Filehandle to write to.
	string - String to write.

    RESULT
	0 if all went well or EOF in case of an error.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FGetC(), IoErr()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    while(*string)
        if(FPutC(file,*string++)<0)
            return EOF;
            
    return 0;
    
    __AROS_FUNC_EXIT
} /* FPuts */
