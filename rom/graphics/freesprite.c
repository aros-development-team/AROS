/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeSprite()
    Lang: english
*/
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, FreeSprite,

/*  SYNOPSIS */
        AROS_LHA(WORD, pick, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 69, Graphics)

/*  FUNCTION
        Free a via GetSprite previously allocated sprite.
        Don't even dare to free a sprite you didn't allocate.   

    INPUTS
        pick - number of sprite in range 0-7

    RESULT
        Sprite is made available for other tasks and the Virtual Sprite
        Machine.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetSprite() ChangeSprite() MoveSprite() graphics/sprite.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  if (pick < 8) {
    UBYTE Mask = (0x01 << pick) ^ 0xff;
    Disable();
    GfxBase->SpriteReserved &= Mask;
    Enable();
  }
  AROS_LIBFUNC_EXIT
} /* FreeSprite */
