/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeExtSpriteA()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

#include "gfxfuncsupport.h"
#include "graphics_intern.h"

#define DEF_POINTER_DEPTH 4

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
	using two private tags: CSTAG_XOffset and CSTAG_YOffset. Possible complete
	sprite engine implementation would need to distinguish between actual sprite
	and mouse cursor, so this can be just a temporary hack.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    OOP_Object *bitmap;
    HIDDT_Color col[DEF_POINTER_DEPTH] = {{0}};
    UWORD i, firstcolor;
    LONG res;
    LONG xoffset = GetTagData(CSTAG_XOffset, 0, tags);
    LONG yoffset = GetTagData(CSTAG_YOffset, 0, tags);

    D(bug("ChangeExtSpriteA(0x%p, 0x%p, 0x%p)\n", vp, oldsprite, newsprite));
    
    /* We have only sprite #0 for the mouse pointer */
    if (newsprite->es_SimpleSprite.num)
        return 0;

    /* Pick up position from old sprite */
    newsprite->es_SimpleSprite.x = oldsprite->es_SimpleSprite.x;
    newsprite->es_SimpleSprite.y = oldsprite->es_SimpleSprite.y;
    D(bug("Sprite position: (%d, %d)\n", newsprite->es_SimpleSprite.x, newsprite->es_SimpleSprite.y));

    /* HIDD_Gfx_SetCursorShape() expects a LUT8 bitmap with palette. A correct bitmap was prepared by
       AllocSpriteDataA(), but it does not have a palette yet (because in original AmigaOS bitmaps
       simply do not contain a palette). So now we have to attach a palette to the bitmap. We pick it
       up from our ViewPort, paying attention to base color number in the ColorMap.

       TODO: Probably we should check here if the palette is already present? Nothing prevents us from
	     supplying a colorful bitmap to AllocSpriteDataA(), why not to support this?
    */
    firstcolor = vp->ColorMap->SpriteBase_Even;
    D(bug("Display has %u colors, pointer starts from %u\n", vp->ColorMap->Count, firstcolor));

    D(bug("Sprite colors:\n"));
    /* This is effectively the same as GetRGB32(), but takes into account that our array
       is 16-bit, not 32-bit, and fills in alpha channel data */
    for (i = 1; i < DEF_POINTER_DEPTH; i++ )
    {
        ULONG red, green, blue;
	
	color_get(vp->ColorMap, &red, &green, &blue, i + firstcolor);
	D(bug("%02u: R 0x%08X, G 0x%08X, B 0x%08X\n", i + firstcolor, red, green, blue));

	col[i].red = red;
	col[i].green = green;
	col[i].blue = blue;
	col[i].alpha = 0x9F9F;
    }

    bitmap = OBTAIN_HIDD_BM(newsprite->es_BitMap);
    D(bug("HIDD bitmap object: 0x%p\n", bitmap));
    HIDD_BM_SetColors(bitmap, col, 0, DEF_POINTER_DEPTH);
    
    /* At last we actually set the cursor shape and make sure it is visible */
    res = HIDD_Gfx_SetCursorShape(SDD(GfxBase)->gfxhidd, bitmap, xoffset, yoffset);
    if (res)
        HIDD_Gfx_SetCursorVisible(SDD(GfxBase)->gfxhidd, TRUE);
    return res;

    AROS_LIBFUNC_EXIT
} /* ChangeExtSpriteA */
