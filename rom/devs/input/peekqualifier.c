/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Input device function PeekQualifier()
    Lang: english
*/
#include "input_intern.h"

/*****************************************************************************

    NAME */
#include <clib/input_protos.h>

	AROS_LH0(UWORD, PeekQualifier,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Library *, InputBase, 7, Input)

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
			    input_lib.fd and clib/input_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,InputBase)
    
    return 0;

    AROS_LIBFUNC_EXIT
} /* PeekQualifier */
