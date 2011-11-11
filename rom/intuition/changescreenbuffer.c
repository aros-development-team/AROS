/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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
    intuition-cooperative way.
    The ScreenBuffer's BitMap will be installed on the specifies screen,
    if possible.
    After a signal from graphics.library, the previously installed
    BitMap will be available for re-use.
    Consult graphics.library/AllocDBufInfo() and
    graphics.library/ChangeVPBitMap() for further information.
 
    INPUTS
    screen - The screen this screenbuffer belongs to
    screenbuffer - The screenbuffer obtained by AllocScreenBuffer()
 
    RESULT
    Non-zero if fuction succeeded, or zero if operation could not be
    performed, eg. if user selects menus or gadgets.
 
    NOTES
    You need not re-install the original ScreenBuffer before closing
    a screen. Just FreeScreenBuffer() all buffers used for that screen.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    AllocScreenBuffer(), FreeScreenBuffer(),
    graphics.library/ChangeVPBitMap()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct IIHData   *iihdata;
    ULONG   	      lock;

    if (!screen) return 0;
    if (!screenbuffer) return 0;

    if (MENUS_ACTIVE) return 0;
    iihdata = (struct IIHData *)((struct IntIntuitionBase *)(IntuitionBase))->InputHandler->is_Data;
    if (iihdata->ActiveGadget) return 0;

    ChangeVPBitMap(&screen->ViewPort,screenbuffer->sb_BitMap,screenbuffer->sb_DBufInfo);

    lock = LockIBase(0);

    screen->BitMap = *screenbuffer->sb_BitMap;
    screen->RastPort.BitMap = screenbuffer->sb_BitMap;

    UnlockIBase(lock);
    
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeScreenBuffer */
