/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function DoCollision()
    Lang: english
*/
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, DoCollision,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 18, Graphics)

/*  FUNCTION

    INPUTS
        rp - pointer to RastPort

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

#warning TODO: Write graphics/DoCollision()
    aros_print_not_implemented ("DoCollision");

    AROS_LIBFUNC_EXIT
} /* DoCollision */
