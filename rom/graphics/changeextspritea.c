/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeExtSpriteA()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <graphics/clip.h>
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <intuition/pointerclass.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

#include "gfxfuncsupport.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH4(LONG, ChangeExtSpriteA,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct ExtSprite *, oldsprite, A1),
        AROS_LHA(struct ExtSprite *, newsprite, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct GfxBase *, GfxBase, 171, Graphics)

/*  FUNCTION

    INPUTS
        vp        - pointer to ViewPort structure that this sprite is
                    relative to, or NULL if relative only top of View
        oldsprite - pointer to the old ExtSprite structure
        newsprite - pointer to the new ExtSprite structure
        tags      - pointer to taglist

    RESULT
        success - 0 if there was an error

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This is a minimal implementation which supports only single sprite #0
        for mouse pointer.
	
	Hosted ports can use host OS' native mouse cursor functions, which need
	to know hotspot coordinates. Passing them to the graphics driver is done
	by forwarding two pointerclass tags: POINTERA_XOffset and POINTERA_YOffset.
	Possible complete sprite engine implementation might need to distinguish
	between actual sprite and mouse cursor, so this can be just a temporary hack.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    OOP_Object *bitmap;
    LONG res;
    LONG xoffset = GetTagData(POINTERA_XOffset, 0, tags);
    LONG yoffset = GetTagData(POINTERA_YOffset, 0, tags);

    D(bug("ChangeExtSpriteA(0x%p, 0x%p, 0x%p)\n", vp, oldsprite, newsprite));
    
    /* We have only sprite #0 for the mouse pointer */
    if (newsprite->es_SimpleSprite.num)
        return 0;

    /* Pick up position from old sprite */
    newsprite->es_SimpleSprite.x = oldsprite->es_SimpleSprite.x;
    newsprite->es_SimpleSprite.y = oldsprite->es_SimpleSprite.y;
    D(bug("Sprite position: (%d, %d)\n", newsprite->es_SimpleSprite.x, newsprite->es_SimpleSprite.y));

    /* Set the cursor shape and make sure it is visible */
    bitmap = OBTAIN_HIDD_BM(newsprite->es_BitMap);
    D(bug("HIDD bitmap object: 0x%p\n", bitmap));
    res = HIDD_Gfx_SetCursorShape(SDD(GfxBase)->gfxhidd, bitmap, xoffset, yoffset);
    if (res)
        HIDD_Gfx_SetCursorVisible(SDD(GfxBase)->gfxhidd, TRUE);
    return res;

    AROS_LIBFUNC_EXIT
} /* ChangeExtSpriteA */
