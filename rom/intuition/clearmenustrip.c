/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(void, ClearMenuStrip,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 9, Intuition)

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

    SANITY_CHECK(window)

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
    window->MenuStrip = NULL;
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    return;

    AROS_LIBFUNC_EXIT
} /* ClearMenuStrip */
