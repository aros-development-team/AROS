/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH8(BOOL, AutoRequest,

         /*  SYNOPSIS */
         AROS_LHA(struct Window    *, window, A0),
         AROS_LHA(struct IntuiText *, body, A1),
         AROS_LHA(struct IntuiText *, posText, A2),
         AROS_LHA(struct IntuiText *, negText, A3),
         AROS_LHA(ULONG             , pFlag, D0),
         AROS_LHA(ULONG             , nFlag, D1),
         AROS_LHA(ULONG             , width, D2),
         AROS_LHA(ULONG             , height, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 58, Intuition)

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
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Window *req;
    ULONG   	   idcmp;
    LONG    	   result;

    EXTENDWORD(width);EXTENDWORD(height);

    req = BuildSysRequest(window,
                          body,
                          posText,
                          negText,
                          pFlag | nFlag,
                          width,
                          height);

    /* req = 0/1 is handled by SysReqHandler */
    while ((result = SysReqHandler(req, &idcmp, TRUE)) == -2)
    {
    }


    if (result == -1)
    {
        result = (idcmp & pFlag) ? 1 : 0;
    }

    FreeSysRequest(req);

    return result;

    AROS_LIBFUNC_EXIT
} /* AutoRequest */
