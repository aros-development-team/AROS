/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Remove a single gadget from a window.
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH2(UWORD, RemoveGadget,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Gadget *, gadget, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 38, Intuition)

/*  FUNCTION
    Remove a gadget from the list of gadgets in a window.
 
    INPUTS
    window - Remove the gadget from this list.
    gadget - Remove this gadget.
 
    RESULT
    The position of the gadget or 0xFFFF if the gadget doesn't
    exist or the gadget is the 65535th of the list.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return RemoveGList (window, gadget, 1);
    
    AROS_LIBFUNC_EXIT
} /* RemoveGadget */
