/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(void, InitRequester,

         /*  SYNOPSIS */
         AROS_LHA(struct Requester *, requester, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 23, Intuition)

/*  FUNCTION
        This function is OBSOLETE and should not be called. To preserve
    compatibility with old programs, calling this function is a no-op.
 
    INPUTS
    requester - The struct Requester to be initialized
 
    RESULT
    None.
 
    NOTES
        This function is obsolete.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    Request(), EndRequest()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_REQUEST(dprintf("InitRequester: req 0x%lx\n", requester));

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    SANITY_CHECK(requester)

    memset(requester, 0, sizeof(struct Requester));

    AROS_LIBFUNC_EXIT
} /* InitRequester */
