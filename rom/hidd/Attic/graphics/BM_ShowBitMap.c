/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Show a bitmap of a graphics hidd
    Lang: english
*/

#include <exec/types.h>
#include <proto/intuition.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH3(BOOL, HIDD_Graphics_ShowBitMap,

/*  SYNOPSIS */
        AROS_LHA(APTR            , bitMap      , A2),
        AROS_LHA(BOOL            , wait        , D2),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 8, GfxHidd)

/*  FUNCTION
        Make a bitmap visible. Some systems allow to show more than one
        bitmap at a time. On these systems, the specified bitmap appears
        before all other bitmaps. If |wait| is |TRUE|, then the call will
        block until the bitmap is made visible (ie. at the next
        |HIDD_Graphics_WaitTOF()|).


    INPUTS
        bitMap  - valid pointer to a bitmap that was created with
                  HIDD_Graphics_CreateBitMap(). Passing a NULL-pointer
                  (meaning "do nothing") is OK.
        wait    - If |TRUE|, then the call will block until the bitmap is
                  made visible (ie. at the next |HIDD_Graphics_WaitTOF()|).
        tagList - for future extensions, set this always to NULL.

    RESULT
        visible - the call return whether bitmap is visible or not if
                  |wait| is |FALSE| or |TRUE| otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Graphics_Bitmap

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_ShowBitMap\n"));
    D(bug("  sorry, not yet implemented\n"));

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_ShowBitMap */
