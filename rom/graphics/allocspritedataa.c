/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AllocSpriteDataA()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/gfx.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>
#include <exec/exec.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct ExtSprite *, AllocSpriteDataA,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, bitmap, A2),
        AROS_LHA(struct TagItem *, taglist, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 170, Graphics)

/*  FUNCTION

    INPUTS
        bitmap - pointer to a bitmap. This bitmap provides the source data
                 for the sprite image
        tags   - pointer to a taglist

    RESULT
        SpritePtr - pointer to a ExtSprite structure,
                    or NULL if there is a failure.
                    You should pass this poionter to FreeSpriteData()
                    when finished with this sprite

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct ExtSprite *sprite;
    
#warning TODO: Write graphics/AllocSpriteDataA()
    aros_print_not_implemented ("AllocSpriteDataA");

    sprite = AllocVec(sizeof(*sprite), MEMF_PUBLIC | MEMF_CLEAR);
    
    return sprite;

    AROS_LIBFUNC_EXIT
} /* AllocSpriteDataA */
