/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeVPBitMap()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(void, ChangeVPBitMap,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct BitMap *, bm, A1),
        AROS_LHA(struct DBufInfo *, db, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 157, Graphics)

/*  FUNCTION

    INPUTS
        vp - pointer to a viewport
        bm - pointer to a BitMap structure. This BitMap structure must be of
             the same layout as the one attached to the viewport
             (same depth, alignment and BytesPerRow)
        db - pointer to a DBufInfo

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

    /* This is a very basic implementation. Screen refresh is completely not in sync with VBlank.
       The main problem here is that AROS completely misses VBlank interrupt. */

    /* Just insert a new bitmap and rebuild current view. MrgCop() is expected to care about
       the rest of stuff. */
    vp->RasInfo->BitMap = bm;
    
    /* On real Amiga we should probably have been called MakeVPort() before this */
    MrgCop(GfxBase->ActiView);

    /* Reply both messages - the displayed bitmap has been swapped */
    ReplyMsg(&db->dbi_SafeMessage);
    ReplyMsg(&db->dbi_DispMessage);

    AROS_LIBFUNC_EXIT
} /* ChangeVPBitMap */
