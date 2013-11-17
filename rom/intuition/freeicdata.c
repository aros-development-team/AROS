/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Support function for icclass and gadgetclass.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "intuition_intern.h"

/*****i***********************************************************************
 
    NAME */
        AROS_LH1(void, FreeICData,

/*  SYNOPSIS */
        AROS_LHA(struct ICData *, icdata, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 146, Intuition)

/*  FUNCTION
        This private function will free the data that belongs to an object
        of the type ICCLASS. It is primarily in as a private function for
        the benefit of intuition.library's gadgetclass implementation,
        which includes an icclass of its own.

    INPUTS
        icdata - The address of a struct ICData

    RESULT
        The data associated will have been freed (including the TagList).

    NOTES
        This function is also present in MorphOS v50, however
        considered private.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    DEBUG_FREEICDATA(dprintf("FreeICData(icdata 0x%lx)\n",icdata));

    SANITY_CHECK(icdata)

    icdata->ic_LoopCounter = 0UL;

    if(icdata->ic_CloneTags)
    {
        FreeTagItems(icdata->ic_CloneTags);
        icdata->ic_CloneTags = NULL;
    }

    AROS_LIBFUNC_EXIT

} /* FreeICData() */
