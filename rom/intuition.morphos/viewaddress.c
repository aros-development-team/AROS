/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function ViewAddress()
    Lang: english
*/
#include <graphics/view.h>
#include <intuition/intuitionbase.h>
#include <aros/libcall.h>
/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH0(struct View *, ViewAddress,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 49, Intuition)

/*  FUNCTION
    Returns the address of the Intuition View structure. This view
    is needed if you want to use any of the graphics, text or animation
    functions in your window that need the pointer to the view structure.
 
    INPUTS
    None
 
    RESULT
    Address of the Intuition View structure
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    graphics.library
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *, IntuitionBase)

    return &(IntuitionBase->ViewLord);

    AROS_LIBFUNC_EXIT
} /* ViewAddress */
