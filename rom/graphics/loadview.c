/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function LoadView()
    Lang: english
*/
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, LoadView,

/*  SYNOPSIS */
        AROS_LHA(struct View *, View, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 37, Graphics)

/*  FUNCTION

    INPUTS
        View - pointer to the View structure which contains the pointer to the
               constructed coprocessor instructions list, or NULL

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

#warning TODO: Write graphics/LoadView()
    aros_print_not_implemented ("LoadView");

    AROS_LIBFUNC_EXIT
} /* LoadView */
