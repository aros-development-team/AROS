/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function LoadView()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, LoadView,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 37, Graphics)

/*  FUNCTION
	Display a new view

    INPUTS
        view - pointer to the View structure which contains the pointer to the
               constructed coprocessor instructions list, or NULL

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(GfxBase->ActiViewCprSemaphore);

    if (GfxBase->ActiView != view)
    {
	GfxBase->ActiView = view;
	DoViewFunction(view, driver_LoadViewPorts, GfxBase);
    }

    ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);

    AROS_LIBFUNC_EXIT
} /* LoadView */
