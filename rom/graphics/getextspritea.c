/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: Graphics function GetExtSpriteA()
*/
#include <aros/debug.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(LONG, GetExtSpriteA,

/*  SYNOPSIS */
        AROS_LHA(struct ExtSprite *, sprite, A2),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 155, Graphics)

/*  FUNCTION

    INPUTS
        sprite - pointer to programmer's ExtSprite (from AllocSpriteData())
        tags   - a standard tag list

    RESULT
        Sprite_number - a sprite number or -1 for an error

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        In AROS, sprite zero is reserved for the mouse cursor and is
        never reserved by this function.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    WORD pick = -1;
    UBYTE SearchMask;

    for (struct TagItem *tag = tags; tag->ti_Tag != TAG_END; ++tag) {
        if (tag->ti_Tag == GSTAG_SPRITE_NUM) {
            pick = tag->ti_Data;
        }
    }
    if (pick > 7) {
        pick = -1;
    } else {
        /*  let nobody else interrupt us while we're looking for a free
         *  sprite
         */
        Disable();

        if (-1 == pick) {
            LONG Count = 1;
            /* user just wants the next available sprite.
             * Since AROS does not set up the Intuition pointer through the
             * sprite API, we skip sprite 0 when searching for a sprite slot. */
            SearchMask = 0x02;

            /* look for the first not allocated sprite */
            while (0 != (GfxBase->SpriteReserved & SearchMask)  &&  Count < 8) {
                SearchMask <<= 1;
                Count++;
            }

            if (8 != Count) {
                /* we were able to allocated a free sprite */
                /* mark the sprite as reserved for the user */
                GfxBase->SpriteReserved |= SearchMask;
                GfxBase->ExtSprites |= SearchMask;
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
                GfxBase->ExtSprites |= SearchMask;
            } else {
                /* no, it's not available any more */
                pick = -1;
            }
        }
        Enable();
    }

    ((struct SimpleSprite *)sprite)->num = pick;

    return pick;
    AROS_LIBFUNC_EXIT
} /* GetExtSpriteA */
