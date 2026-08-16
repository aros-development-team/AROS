/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Return the semaphore arbitrating GfxBase->TextFonts.
          Private function for diskfont.library support.
*/

#include "graphics_intern.h"

AROS_LH0(struct SignalSemaphore *, GetFontSemaphore,
         struct GfxBase *, GfxBase, 202, Graphics)
{
    AROS_LIBFUNC_INIT

    /* The documented "Forbid() to walk gb_TextFonts" contract does not
     * exclude other cores; walkers take this instead. */
    return &PrivGBase(GfxBase)->fontsem;

    AROS_LIBFUNC_EXIT
}
