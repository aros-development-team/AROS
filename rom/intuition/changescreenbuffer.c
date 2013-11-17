/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Desc: Intuition function ChangeScreenBuffer()
    Lang: english
*/
#include <proto/graphics.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "menus.h"

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

        AROS_LH2(ULONG, ChangeScreenBuffer,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *      , screen, A0),
        AROS_LHA(struct ScreenBuffer *, screenbuffer, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 130, Intuition)

/*  FUNCTION
        Do double or multiple buffering on an intuition screen in an
        intuition-cooperative way. The ScreenBuffer's BitMap will be
        installed on the specified screen, if possible. After a signal from
        graphics.library, the previously installed BitMap will be available
        for re-use. Consult graphics.library/AllocDBufInfo() and
        graphics.library/ChangeVPBitMap() for further information.

    INPUTS
        screen - The screen this screenbuffer belongs to
        screenbuffer - The screenbuffer obtained by AllocScreenBuffer()

    RESULT
        Non-zero if fuction succeeded, or zero if operation could not be
        performed, e.g. if user selects menus or gadgets.

    NOTES
        You need not re-install the original ScreenBuffer before closing
        a screen. Just FreeScreenBuffer() all buffers used for that screen.

    EXAMPLE

    BUGS

    SEE ALSO
        AllocScreenBuffer(), FreeScreenBuffer(),
        graphics.library/ChangeVPBitMap()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    //struct IIHData   *iihdata;
    ULONG lock;

    if (!screen) return 0;
    if (!screenbuffer) return 0;

    #if 0
    if (MENUS_ACTIVE) return 0;
    iihdata = (struct IIHData *)((struct IntIntuitionBase *)(IntuitionBase))->InputHandler->is_Data;
    if (iihdata->ActiveGadget) return 0;
    #endif

    lock = LockIBase(0);

    ChangeVPBitMap(&screen->ViewPort,screenbuffer->sb_BitMap,screenbuffer->sb_DBufInfo);

    screen->RastPort.BitMap = screenbuffer->sb_BitMap;
    UpdateScreenBitMap(screen, IntuitionBase);

    UnlockIBase(lock);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeScreenBuffer */

void UpdateScreenBitMap(struct Screen *screen, struct IntuitionBase *IntuitionBase)
{
    int i;
    struct BitMap *bm = screen->RastPort.BitMap;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    /* Patch up the obsolete screen bitmap as best we can for old programs
     */
    screen->BitMap_OBSOLETE.BytesPerRow = GetBitMapAttr(bm, BMA_WIDTH) / 8;
    screen->BitMap_OBSOLETE.Rows = GetBitMapAttr(bm, BMA_HEIGHT);
    screen->BitMap_OBSOLETE.Flags = BMF_INVALID | (GetBitMapAttr(bm, BMA_FLAGS) & BMF_STANDARD);
    screen->BitMap_OBSOLETE.pad = 0;
    screen->BitMap_OBSOLETE.Depth = GetBitMapAttr(bm, BMA_DEPTH);
    for (i = 0; i < 8; i++) {
        /* And for *really* old programs, copy the plane pointers,
         * if possible
         */
        if (screen->BitMap_OBSOLETE.Flags & BMF_STANDARD)
            screen->BitMap_OBSOLETE.Planes[i] = bm->Planes[i];
        else
            screen->BitMap_OBSOLETE.Planes[i] = NULL;
    }
}
