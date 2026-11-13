/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function SetMaxPen()
*/
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(void, SetMaxPen,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp    , A0),
        AROS_LHA(ULONG            , maxpen, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 165, Graphics)

/*  FUNCTION
        Set the maximum pen value for a rastport. This will instruct the
        graphics.library that the owner of the rastport will not be rendering
        in any colors whose index is >maxpen. Therefore speed optimizations
        on certain operations are possible and will be done.

        Basically this call sets the rastport mask, if this would improve speed.
        On devices where masking would slow things down (chunky pixels), it will
        be a no-op.

    INPUTS
        rp     = pointer to a valid RastPort structure
        maxpen = longword pen value (valid range here: 0..255)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetWriteMask()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* maxpen==0 is nonsense */
    if (maxpen == 0)
        return;

    /*
     * Compute rp->Mask so that all bits up to the highest set bit in maxpen
     * are set:
     *   mask = (1<<(floor(log2(maxpen))+1)) - 1
     *
     * maxpen is guaranteed to be <= 255, so the result always fits in 8 bits.
     *
     * You can choose the implementation:
     *   - default: builtin clz-based (fast, clear)
     *   - define SETMAXPEN_USE_SMEAR to use the portable "bit smear" method
     */
#if defined(SETMAXPEN_USE_SMEAR)

    /* Portable, very fast, no builtins required */
    {
        ULONG v = maxpen;    /* 1..255 */
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        rp->Mask = (UBYTE)v;
    }

#else /* !SETMAXPEN_USE_SMEAR */

# if defined(__GNUC__) || defined(__clang__)

    /* Builtin-based: highest bit via count-leading-zeros */
    {
        unsigned k = 31u - (unsigned)__builtin_clz((unsigned)maxpen); /* maxpen != 0 */
        rp->Mask = (UBYTE)((1u << (k + 1u)) - 1u);
    }

# else

    /* Fallback for non-GNU/Clang compilers */
    {
        UBYTE mask = 0;
        ULONG v = maxpen;    /* 1..255 */
        while (v != 0)
        {
            v >>= 1;
            mask = (UBYTE)((mask << 1) | 1u);
        }
        rp->Mask = mask;
    }

# endif /* compiler */

#endif /* SETMAXPEN_USE_SMEAR */

    AROS_LIBFUNC_EXIT
} /* SetMaxPen */
