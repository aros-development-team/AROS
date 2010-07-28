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

	With vp set to NULL the function always fails at the moment.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    OOP_Object *bitmap;
    struct monitor_driverdata *mdd;
    LONG res;

    D(bug("ChangeExtSpriteA(0x%p, 0x%p, 0x%p)\n", vp, oldsprite, newsprite));

    /* We have only sprite #0 for the mouse pointer */
    if (newsprite->es_SimpleSprite.num)
        return 0;

    /* Pick up position from old sprite */
    newsprite->es_SimpleSprite.x = oldsprite->es_SimpleSprite.x;
    newsprite->es_SimpleSprite.y = oldsprite->es_SimpleSprite.y;
    D(bug("Sprite position: (%d, %d)\n", newsprite->es_SimpleSprite.x, newsprite->es_SimpleSprite.y));

    bitmap = OBTAIN_HIDD_BM(newsprite->es_BitMap);
    D(bug("HIDD bitmap object: 0x%p\n", bitmap));

    if (vp) {
        /* Pick up display driver from ViewPort's bitmap */
        mdd = GET_BM_DRIVERDATA(vp->RasInfo->BitMap);
	res = HIDD_Gfx_SetCursorShape(mdd->gfxhidd, bitmap, 0, 0);
	if (res)
	    HIDD_Gfx_SetCursorVisible(mdd->gfxhidd, TRUE);
    } else
	/* TODO: NULL ViewPort means Amiga(tm) chipset display */
        res = FALSE;

    RELEASE_HIDD_BM(bitmap, newsprite->es_BitMap);
    return res;

    AROS_LIBFUNC_EXIT
} /* ChangeExtSpriteA */
