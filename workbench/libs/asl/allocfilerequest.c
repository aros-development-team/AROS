/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "asl_intern.h"

/*****************************************************************************

    NAME */
#include <proto/asl.h>
#include <clib/asl_protos.h>

	AROS_LH0(struct FileRequester *, AllocFileRequest,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Library *, AslBase, 5, Asl)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,AslBase)

    return ((struct FileRequester *)AllocAslRequest(ASL_FileRequest, NULL));

    AROS_LIBFUNC_EXIT
} /* AllocFileRequest */
