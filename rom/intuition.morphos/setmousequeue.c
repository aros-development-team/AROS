/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(LONG, SetMouseQueue,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(UWORD          , queuelength, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 83, Intuition)

/*  FUNCTION
    Change the number of mouse messages for your window to be allowed
    to be outstanding.
 
    INPUTS
    window - the window
    queuelength - the number of mouse messages to be allowed to be
        outstanding
 
    RESULT
    Returns -1 if the window is unknown otherwise the old value of the
    queuelength is returned.
 
    NOTES
    There should be a function for changing the repeat key queue limit, too.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    OpenWindow()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    LONG result;

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    SANITY_CHECKR(window,-1)

    result = ((struct IntWindow *)window)->mousequeue;
    ((struct IntWindow *)window)->mousequeue = queuelength;

    return result;

    AROS_LIBFUNC_EXIT
} /* SetMouseQueue */
