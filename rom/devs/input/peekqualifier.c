/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
	struct Device *, InputBase, 7, Input)

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
    AROS_LIBBASE_EXT_DECL(struct Device *,InputBase)
    
    return ((struct inputbase *)InputBase)->ActQualifier;

    AROS_LIBFUNC_EXIT
} /* PeekQualifier */
