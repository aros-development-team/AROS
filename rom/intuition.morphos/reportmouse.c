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

AROS_LH2(void, ReportMouse,

         /*  SYNOPSIS */
         AROS_LHA(LONG           , flag, D0),
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 39, Intuition)

/*  FUNCTION
    Enable or disable the window flag REPORTMOUSE. If the flag is
    set, you will receive IDCMP event every time the user moves
    the mouse.
 
    INPUTS
    flag - Enable (TRUE) or disable (FALSE) the reports.
    window - Do it in this window.
 
    RESULT
    None.
 
    NOTES
    As you might have noticed, the arguments are twisted.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_REPORTMOUSE(dprintf("ReportMouse: Window 0x%lx Flag %d\n", window, flag));

    SANITY_CHECK(window)

    if (flag) AROS_ATOMIC_OR(window->Flags, WFLG_REPORTMOUSE);
    else      AROS_ATOMIC_AND(window->Flags, ~WFLG_REPORTMOUSE);
    

    AROS_LIBFUNC_EXIT
} /* ReportMouse */
