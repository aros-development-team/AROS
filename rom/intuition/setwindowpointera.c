/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function SetWindowPointerA()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, SetWindowPointerA,

/*  SYNOPSIS */
	AROS_LHA(struct Window * , window , A0),
	AROS_LHA(struct TagItem *, taglist, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 136, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetWindowPointerA()
    aros_print_not_implemented ("SetWindowPointerA");

    AROS_LIBFUNC_EXIT
} /* SetWindowPointerA */
