/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/exec.h>
#include <cybergraphx/cybergraphics.h>
#include "intuition_intern.h"
/*****************************************************************************
 
    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

AROS_LH3(struct ScreenBuffer *, AllocScreenBuffer,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, screen, A0),
         AROS_LHA(struct BitMap *, bitmap, A1),
         AROS_LHA(ULONG          , flags, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 128, Intuition)

/*  FUNCTION
    Allocate a ScreenBuffer (and BitMap) for double or multiple
    buffering in Intuition screens. Use this function to obtain a
    ScreenBuffer for the screen's initial BitMap and for all other
    BitMaps you want to swap in.
    
    This function also allocates a DBufInfo from graphics.library
    The returned ScreenBuffer contains a pointer to that DBufInfo.
    See graphics.library/AllocDBufInfo() for more information on
    how to use this struct to obtain info when it is safe to render
    into an old buffer and when to switch.
 
    INPUTS
    screen - Screen to double-buffer
    bitmap - You may pre-allocate a BitMap for CUSTOMBITMAP screens,
        and pass the pointer to get a ScreenBuffer referring to it.
        If you specify NULL, intuition will allocate the BitMap for
        you. For non-CUSTOMBITMAP screens this parameter must be NULL.
    flags - A combination of these flags:
        SB_SCREEN_BITMAP for non-CUSTOMBITMAP screens to get a
        ScreenBuffer referring to the screen's actual BitMap
        (For CUSTOMBITMAP screens just pass the BitMap you used for
        OpenScreen() as the bitmap parameter)
        SB_COPY_BITMAP to copy the screen's BitMap intto the
        ScreenBuffer's BitMap. Use this to get intuition rendered
        stuff into your bitmap (such as menu-bars or gadgets).
        May be omitted if the screen has no intuition rendered stuff,
        as well as for allocating a ScreenBuffer for the screen's
        initial BitMap.
        
 
    RESULT
    Pointer to the allocated ScreenBuffer or NULL if function failed.
 
    NOTES
    You may render into the resulting BitMap.
    Use the sb_DBufInfo field to access graphics.library's ViewPort
    buffering features to e.g check if it is safe to reuse the previous
    BitMap. Otherwise you risk to write into the on-screen BitMap and
    damage menu or gadget rendering.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    FreeScreenBuffer(), ChangeScreenBuffer()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct IntScreenBuffer *ScreenBuffer = NULL;

    DEBUG_ALLOCSCREENBUFFER(dprintf("AllocScreenBuffer: Screen 0x%lx BitMap 0x%lx Flags 0x%lx\n",
                                    screen, bitmap, flags));

    if (screen)
    {
        if ((ScreenBuffer = AllocMem(sizeof (struct IntScreenBuffer),
                                     MEMF_CLEAR)))
        {
            if ((ScreenBuffer->sb.sb_DBufInfo =
                        AllocDBufInfo(&screen->ViewPort)))
            {
                if (!bitmap)
                {
                    /* Get a bitmap */
                    if (flags & SB_SCREEN_BITMAP)
                    {
                        bitmap = screen->RastPort.BitMap;
                    }
                    else
                    {
                        ScreenBuffer->free_bitmap = TRUE;

                        bitmap = AllocBitMap(GetBitMapAttr(screen->RastPort.BitMap,BMA_WIDTH),
                                             GetBitMapAttr(screen->RastPort.BitMap,BMA_HEIGHT),
                                             GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH),
                                             BMF_MINPLANES|BMF_DISPLAYABLE|BMF_CLEAR,
                                             screen->RastPort.BitMap);

                        if (NULL == bitmap)
                        {
                            FreeDBufInfo(ScreenBuffer->sb.sb_DBufInfo);
                            FreeMem(ScreenBuffer, sizeof(struct IntScreenBuffer));
                            DEBUG_ALLOCSCREENBUFFER(dprintf("AllocScreenBuffer: failed\n"));
			    
                            return NULL;
                        }
                    }
                }

                ScreenBuffer->sb.sb_BitMap = bitmap;

                if (flags & SB_COPY_BITMAP)
                {
                    BltBitMap(screen->RastPort.BitMap,
                              0,
                              0,
                              bitmap,
                              0,
                              0,
                              screen->Width,
                              screen->Height,
                              0x0c0, /* copy */
                              ~0,
                              NULL);
                }

                DEBUG_ALLOCSCREENBUFFER(dprintf("AllocScreenBuffer: ScreenBuffer 0x%lx BitMap 0x%lx DBufInfo 0x%lx\n",
                                                ScreenBuffer, bitmap, ScreenBuffer->sb.sb_DBufInfo));

                return &ScreenBuffer->sb;
            }

            FreeMem(ScreenBuffer, sizeof(struct IntScreenBuffer));
        }
    }

    DEBUG_ALLOCSCREENBUFFER(dprintf("AllocScreenBuffer: no mem\n"));

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocScreenBuffer */
