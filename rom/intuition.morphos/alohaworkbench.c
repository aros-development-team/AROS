/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH1(void, AlohaWorkbench,

         /*  SYNOPSIS */
         AROS_LHA(struct MsgPort *, wbmsgport, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 67, Intuition)

/*  FUNCTION
        The WorkBench program wants to call this function to signal
        Intuition that it is active or shutting down.
        Intuition then uses the MsgPort to tell the WorkBench to open or
        close its windows if the user called OpenWorkbench() or
        CloseWorkbench().

        When the MsgPort is non-NULL Intuition will send IntuiMessages to
        it with the Class field set to WBENCHMESSAGE and Code field set to
        either WBENCHOPEN or WBENCHCLOSE. Intuition assumes that when the
        WorkBench task replies this messages, it already has opened/closed
        its windows.

    INPUTS
        wbmsgport - The MsgPort of the (initialized) WorkBench task or
                    NULL if the task is shutting down.

    RESULT
        None.

    NOTES
        This function is obsolete and should not be used directly by the
        Workbench Application. Use workbench.library/RegisterWorkbench()
        instead!

    EXAMPLE

    BUGS

    SEE ALSO
        workbench.library/RegisterWorkbench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Save MsgPort into private IntuitionBase */
    DEBUG_ALOHAWORKBENCH(dprintf("AlohaWorkbench: MsgPort %p\n",
                     wbmsgport));
    GetPrivIBase(IntuitionBase)->WorkBenchMP = wbmsgport;

    AROS_LIBFUNC_EXIT
} /* AlohaWorkbench */
