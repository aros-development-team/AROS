/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetChipRev()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, SetChipRev,

/*  SYNOPSIS */
        AROS_LHA(ULONG, ChipRev, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 148, Graphics)

/*  FUNCTION

    INPUTS
        ChipRev - Chip Rev that you would like to be enabled

    RESULT
        chiprevbits - Actual bits set in GfxBase->ChipRevBits0

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Write graphics/SetChipRev() */
    aros_print_not_implemented ("SetChipRev");

    return GfxBase->ChipRevBits0;

    AROS_LIBFUNC_EXIT
} /* SetChipRev */
