/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Intuition function EasyRequestArgs()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH4(LONG, EasyRequestArgs,

/*  SYNOPSIS */
	AROS_LHA(struct Window     *, window, A0),
	AROS_LHA(struct EasyStruct *, easyStruct, A1),
	AROS_LHA(ULONG             *, idcmpPtr, A2),
	AROS_LHA(APTR               , args, A3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 98, Intuition)

/*  FUNCTION
	Not implemented. Always returns -1 (fail).

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return -1;

    AROS_LIBFUNC_EXIT
} /* EasyRequestArgs */
