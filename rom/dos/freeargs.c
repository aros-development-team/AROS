/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/rdargs.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, FreeArgs,

/*  SYNOPSIS */
	AROS_LHA(struct RDArgs *, args, D1),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* ReadArgs() failed. Clean everything up. */
    FreeVec(((struct DAList *)args->RDA_DAList)->ArgBuf);
    FreeVec(((struct DAList *)args->RDA_DAList)->StrBuf);
    FreeVec(((struct DAList *)args->RDA_DAList)->MultVec);
    FreeVec((struct DAList *)args->RDA_DAList);
    FreeVec(args);

    AROS_LIBFUNC_EXIT
} /* FreeArgs */
