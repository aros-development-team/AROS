/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <strings.h>

#include "identify_intern.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH0(void, IdHardwareUpdate,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 10, Identify)

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

    // FIXME: need Semaphore?
    IdentifyBase->dirtyflag = TRUE;
    memset(&IdentifyBase->hwb, 0, sizeof IdentifyBase->hwb);

    AROS_LIBFUNC_EXIT
} /* IdHardwareUpdate */
