/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <graphics/rastport.h>
#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

AROS_LH2(void, FreeScreenBuffer,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *      , screen, A0),
         AROS_LHA(struct ScreenBuffer *, screenbuffer, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 129, Intuition)

/*  FUNCTION
    Frees a ScreenBuffer allocated by AllocScreenBuffer() and releases
    associated resources. You have to call this before closing your
    screen.
 
    INPUTS
    screen - The screen this screenbuffer belongs to
    screenbuffer - The screenbuffer obtained by AllocScreenBuffer()
        It is safe to pass NULL.
 
    RESULT
    None.
 
    NOTES
    When used SB_SCREEN_BITMAP on allocating the ScreenBuffer
    (ie. the ScreenBuffer only refers to the screen's BitMap) you must
    FreeScreenBuffer() the ScreenBuffer before closing the screen.
    Intuition will recognize when FreeScreenBuffer() is called for the
    currently installed ScreenBuffer that it must not free the BitMap.
    This is left to the CloseScreen() function.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    AllocScreenBuffer(), ChangeScreenBuffer()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_FREESCREENBUFFER(dprintf("FreeScreenBuffer: Screen 0x%lx ScreenBuffer 0x%lx\n",
                                   screen, screenbuffer));

    screen = screen;       /* shut up the compiler */

    if (screenbuffer)
    {
        FreeDBufInfo(screenbuffer->sb_DBufInfo);
	
        if (((struct IntScreenBuffer *)screenbuffer)->free_bitmap)
            FreeBitMap(screenbuffer->sb_BitMap);

        FreeMem(screenbuffer, sizeof(struct IntScreenBuffer));
    }

    AROS_LIBFUNC_EXIT
} /* FreeScreenBuffer */
