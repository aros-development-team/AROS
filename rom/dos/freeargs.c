/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:47  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/rdargs.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(void, FreeArgs,

/*  SYNOPSIS */
	__AROS_LHA(struct RDArgs *, args, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 143, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

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

    /* ReadArgs() failed. Clean everything up. */
    FreeVec(((struct DAList *)args->RDA_DAList)->ArgBuf);
    FreeVec(((struct DAList *)args->RDA_DAList)->StrBuf);
    FreeVec(((struct DAList *)args->RDA_DAList)->MultVec);
    FreeVec((struct DAList *)args->RDA_DAList);
    FreeVec(args);

    __AROS_FUNC_EXIT
} /* FreeArgs */
