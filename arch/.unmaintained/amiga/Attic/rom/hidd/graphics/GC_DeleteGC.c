/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Delete a graphics context
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>

#include "gfxhidd_internIntuition.h"
#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH2(VOID, HIDD_Graphics_DeleteGC,

/*  SYNOPSIS */
        AROS_LHA(APTR            , gc          , A2),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 12, GfxHidd)

/*  FUNCTION
        Return a graphics context for reuse. You can also dispose the gc
        with |DisposeObject()|. The bitmap which is connected to this
        graphics context will not be deleted.

    INPUTS
        gc - valid pointer to a graphics context that was created with
                  HIDD_Graphics_CreateGC(). Passing a NULL-pointer
                  (meaning "do nothing") is OK.
        tagList - for future extensions, set this always to NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_GC

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define GC ((struct hGfx_gc *) gc)
#define GCINT ((struct hGfx_gcInt *) gc)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_DeleteGC\n"));
    /* D(bug("  sorry, not yet implemented\n")); */

    if(gc)
    {
        if(GC->bitMap)
        {
            if(((struct hGfx_bitMapInt *)GC->bitMap)->bitMap)
            {
                FreeVec(GCINT->rPort);
            }
        }
        FreeVec(gc);
    }
    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_DeleteGC */
