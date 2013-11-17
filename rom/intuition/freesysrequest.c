/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

        AROS_LH1(void, FreeSysRequest,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 62, Intuition)

/*  FUNCTION
        Frees a requester made with BuildSysRequest() or
        BuildEasyRequestArgs().

    INPUTS
        window - The requester to be freed. May be NULL or 1.

    RESULT

    NOTES

    EXAMPLE

    BUGS
        BuildSysRequest() requesters not supported, yet.

    SEE ALSO
        BuildSysRequest(), BuildEasyRequestArgs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    freesysreq_intern(window, IntuitionBase);
    
    AROS_LIBFUNC_EXIT
}
