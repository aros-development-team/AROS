/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, FreeScreenDrawInfo,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen   *, screen, A0),
         AROS_LHA(struct DrawInfo *, drawInfo, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 116, Intuition)

/*  FUNCTION
    Tell intuition that you have finished work with struct DrawInfo
    returned by GetScreenDrawInfo()
 
    INPUTS
    screen - The screen you passed to GetScreenDrawInfo()
    drawInfo - The DrawInfo structure returned by GetScreenDrawInfo()
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    GetScreenDrawInfo()
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_FREESCREENDRAWINFO(dprintf("FreeScreenDrawInfo: Screen 0x%lx DrawInfo 0x%lx\n",
                                     screen, drawInfo));

    /* shut up the compiler */
    IntuitionBase = IntuitionBase;
    drawInfo = drawInfo;
    screen = screen;

    return;

    AROS_LIBFUNC_EXIT
} /* FreeScreenDrawInfo */
