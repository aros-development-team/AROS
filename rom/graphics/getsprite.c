/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetSprite()
    Lang: english
*/
#include <graphics/gfxbase.h>
#include <graphics/sprite.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(WORD, GetSprite,

/*  SYNOPSIS */
	AROS_LHA(struct SimpleSprite *, sprite, A0),
	AROS_LHA(WORD                 , pick  , D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 68, Graphics)

/*  FUNCTION          	
	Try to get a hardware sprite for the simple sprite manager.
	There are eight sprites available in the system and by calling
	this function you can allocate one for yourself. You have to
	call this function before talking to other sprite routines.
	If you want a 15 color sprite, you must allocate both sprites
	(see the manual!) and set the SPRITE_ATTACHED bit in the
	odd sprite's posctldata array.
	

    INPUTS
	sprite - pointer to a SimpleSprite structure
	pick   - number of the sprite (0-7) of -1 if you just want
		 the next available sprite

    RESULT
	-1 - if the selected sprite is not available (pick was 0-7) or
	     no further sprites are available (pick was -1). -1 will
	     also be found in the SimpleSprite structure.
	0-7: The sprite number of your allocated sprite. The number will
	     also be foung in the SimpleSprite structure.

    NOTES

    EXAMPLE

    BUGS
	On some machines this will never return anything else than -1!

    SEE ALSO
	FreeSprite() ChangeSprite() MoveSprite() GetSprite() graphics/sprite.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  UBYTE SearchMask;

  if (pick > 7 && pick != -1) {
    pick = -1;
  } else {
    /*  let nobody else interrupt us while we're looking for a free
     *  sprite
     */
    Disable();

    if (-1 == pick) {
      LONG Count = 0;
      /* user just wants the next available sprite */
      SearchMask = 0x01;

      /* look for the first not allocated sprite */
      while (0 != (GfxBase->SpriteReserved & SearchMask)  &&  Count < 8) {
        SearchMask <<= 1;
        Count++;
      }

      if (8 != Count) {
        /* we were able to allocated a free sprite */
        /* mark the sprite as reserved for the user */
        GfxBase->SpriteReserved |= SearchMask;
        pick = Count;
      } else {
        /* no sprite was available for the user */
        pick = -1;
      }

    } else {
      /* user wants one specific sprite */
      SearchMask = 0x01 << pick;

      /* is that sprite still available? */
      if (0 == (GfxBase->SpriteReserved & SearchMask) ) {
        /* yes -> mark it as reserved for the user */
        GfxBase->SpriteReserved |= SearchMask;
      } else {
        /* no, it's not available any more */
        pick = -1;
      }
    }
    Enable();
  }
  
  sprite->num = pick;

  return pick;
  AROS_LIBFUNC_EXIT
} /* GetSprite */
