/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a graphics context for a graphics hidd
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>

#include <exec/memory.h>
#include <hidd/graphics.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH2(APTR, HIDD_Graphics_CreateGC,

/*  SYNOPSIS */
        AROS_LHA(APTR            , bitMap      , A2),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 11, GfxHidd)

/*  FUNCTION
        Create a graphics context. The graphics context is used to store
        information which is needed for more than one command (for example
        the color or the drawmode).

    INPUTS
        bitmap - valid pointer to a bitmap that was created with
                 HIDD_Graphics_CreateMap(). This bitmap is connected
                 with the created graphics context. If this pointer is
                 NULL the creation of the graphics context will fail.

    RESULT
        gc - pointer to the created graphics context, or NULL if it was not
             possible to create the graphics context.
        tagList - for future extensions, set this always to NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_GC

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define BM ((struct hGfx_bitMap *) bitMap)
#define GC ((struct hGfx_gc *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_CreateGC\n"));
    /*D(bug("  sorry, not yet implemented\n")); */

    struct hGfx_gc *gc = NULL;

    if(bitMap)
    {
        gc = AllocVec(sizeof(struct hGfx_gc), MEMF_CLEAR | MEMF_PUBLIC);
    }

    if(gc)
    {
        /* Initialize GC */
        GC->bitMap    = bitMap;   /* bitmap to which this gc is connected    */
        GC->fg        = 1;        /* foreground color                        */
        GC->bg        = 0;        /* background color                        */
        GC->drMode    = HIDDV_GC_DrawMode_Copy;    /* drawmode               */
        GC->font      = NULL;     /* current fonts                           */
        GC->colMask   = -1;       /* ColorMask prevents some color bits from */
                                  /* changing                                */
        GC->linePat   = -1;       /* LinePattern                             */
        GC->planeMask = NULL;     /* Pointer to a shape bitMap               */
    }

    return(gc);

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_CreateGC */
