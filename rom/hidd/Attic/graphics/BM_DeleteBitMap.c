/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Delete a bitmap from a graphics hidd
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>

#include "gfxhidd_intern.h"
#define DEBUG 1
#include <aros/debug.h>

/*********************************************************************

    NAME */
#include <proto/gfxhidd.h>
#include <utility/tagitem.h>

        AROS_LH2(VOID, HIDD_Graphics_DeleteBitMap,

/*  SYNOPSIS */
        AROS_LHA(APTR            , bitMap      , A2),
        AROS_LHA(struct TagItem *, tagList     , A3),

/*  LOCATION */
        struct Library *, GfxHiddBase, 7, GfxHidd)

/*  FUNCTION
        Delete a bitmap that was created with HIDD_Graphics_CreateBitMap().

    INPUTS
        bitmap  - valid pointer to a bitmap that was created with
                  HIDD_Graphics_CreateBitMap(). Passing a NULL-pointer
                  (meaning "do nothing") is OK.
        tagList - for future extensions, set this always to NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_Graphics_Bitmap

    INTERNALS

    HISTORY
        05-04-98    drieling created
***************************************************************************/

#define BM ((struct hGfx_bitMapInt *) bitMap)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxHiddBase_intern *,GfxHiddBase)

    D(bug("HIDD_Graphics_DeleteBitMap\n"));
    D(bug("  sorry, not yet implemented\n"));

    AROS_LIBFUNC_EXIT

} /* HIDD_Graphics_DeleteBitMap */
