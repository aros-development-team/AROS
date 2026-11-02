/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdlib.h>

#include "mui.h"
#include "penspec.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, MUI_ObtainPen,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_RenderInfo *, mri, A0),
        AROS_LHA(struct MUI_PenSpec *, spec, A1),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 26, MUIMaster)

/*  FUNCTION
        Resolve a MUI pen specification against the rendering context.
        MUI and system pens refer to existing pens. A colormap specification
        selects an existing entry; negative indices count from the end of
        the colormap. An RGB specification uses ObtainBestPenA(), falling
        back to FindColor() when allocation fails.

    INPUTS
        mri - Initialized rendering context with a colormap and the draw
              information required by the selected specification.
        spec - NUL-terminated pen specification in a MUI_PenSpec structure.
               Supported prefixes are m (MUI pen), p (colormap index),
               s (system pen), and r (three hexadecimal RGB components).
               RGB components are separated by commas; r may be omitted.
        flags - Ignored by this implementation; pass zero.

    RESULT
        A pen token, or -1 if mri, spec, or the colormap is NULL, parsing
        fails, or a checked pen index is out of range. Use MUIPEN(token)
        to obtain the drawing pen number. Bit 16 records an allocated pen.

    NOTES
        Pass the complete token, not its masked pen number, to
        MUI_ReleasePen() with the same rendering context when finished.
        Keep that context and its colormap alive until the pen is released.

    BUGS
        System-pen indices are not range-checked. Supply a valid index into
        the rendering context's DrawInfo pen array. The RGB parser does
        not fully validate separators or the end of the specification.

    SEE ALSO
        MUI_ReleasePen(), graphics.library/ObtainBestPenA(),
        graphics.library/FindColor()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG retval = -1;
    struct MUI_PenSpec_intern intern;
    
    if (!spec || !mri || !mri->mri_Colormap) return -1;

    if (!zune_pen_spec_to_intern(spec, &intern))
        return -1;
    if (!zune_penspec_setup(&intern, mri))
        return -1;
    retval = intern.p_pen;
    if ((retval != -1) && (intern.p_is_allocated))
    {
        /* flag to indicate that ReleasePen() needs to be called
           in MUI_ReleasePen() */
                       
        retval |= 0x10000;
    }

    return retval;

    AROS_LIBFUNC_EXIT

} /* MUIA_ObtainPen */
