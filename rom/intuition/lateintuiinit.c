/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Late initialization of intuition.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <exec/alerts.h>
#include <exec/types.h>

#include <aros/config.h>
#include <aros/libcall.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

AROS_LH1(BOOL, LateIntuiInit,

         /*  SYNOPSIS */
         AROS_LHA(APTR, data, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 150, Intuition)

/*  FUNCTION
    This function permits late initalization
    of intuition (After dos and after graphics hidds are setup,
    but before starup-sequence is run.
    Can be used to open workbench screen.
 
    INPUTS
    data - unused for now.
 
    RESULT
    success - TRUE if initialization went, FALSE otherwise.
 
    NOTES
    This function is private and AROS specific.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* shut up the compiler */
    data = data;

    /* setup pointers */
    GetPrivIBase(IntuitionBase)->DefaultPointer = MakePointerFromPrefs
    (
        IntuitionBase, GetPrivIBase(IntuitionBase)->ActivePreferences
    );
    GetPrivIBase(IntuitionBase)->BusyPointer = MakePointerFromPrefs
    (
        IntuitionBase, GetPrivIBase(IntuitionBase)->ActivePreferences
    );

    if
    (
           !GetPrivIBase(IntuitionBase)->DefaultPointer 
        || !GetPrivIBase(IntuitionBase)->BusyPointer
    )
    {
        return FALSE;
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* LateIntuiInit() */
