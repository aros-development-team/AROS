/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(VOID, FreeDTMethods,

/*  SYNOPSIS */
	AROS_LHA(APTR, methods, A0),

/*  LOCATION */
	struct Library *, DataTypesBase, 47, DataTypes)

/*  FUNCTION

    Free array obtained from CopyDTMethods() or CopyDTTriggerMethods().

    INPUTS

    methods  --  array of methods; may be NULL

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CopyDTMethods(), CopyDTTriggerMethods()

    INTERNALS

    HISTORY

    2.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeVec(methods);

    AROS_LIBFUNC_EXIT
} /* FreeDTMethods */
