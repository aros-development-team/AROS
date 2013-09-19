/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/


#include <proto/exec.h>
#include <proto/intuition.h>
#include "asl_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, ActivateAslRequest,

/*  SYNOPSIS */
        AROS_LHA(APTR, requester, A0),

/*  LOCATION */
        struct Library *, AslBase, 14, Asl)

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

    struct ReqNode *reqnode;

    if (!requester) return;

    ObtainSemaphore( &(ASLB(AslBase)->ReqListSem));

    reqnode = FindReqNode(requester, ASLB(AslBase));
    if (reqnode)
    {
        if (reqnode->rn_ReqWindow)
        {
            ActivateWindow(reqnode->rn_ReqWindow);

            /* Not sure if we need to wait for it to actually become active here before returning or not?! */

        } /* if (reqnode->rn_ReqWindow) */

    } /* if (reqnode) */

    ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));

    AROS_LIBFUNC_EXIT

} /* ActivateAslRequest */
