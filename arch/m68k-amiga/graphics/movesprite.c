/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function MoveSprite()
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <graphics/monitor.h>
#include <proto/exec.h>
#include <oop/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

#include <hidd/amigavideo.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH4(void, MoveSprite,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct SimpleSprite *, sprite, A1),
        AROS_LHA(WORD, x, D0),
        AROS_LHA(WORD, y, D1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 71, Graphics)

/*  FUNCTION
        Move sprite to a new position on the screen. Coordinates
        are specified relatively to given ViewPort, or relatively
        to the entire View (physical display) if the ViewPort is NULL.

        This function works also with extended sprites, since
        struct SimpleSprite is a part of struct ExtSprite.

    INPUTS
        vp     - a ViewPort for relative sprite positioning or NULL
        sprite - a pointer to a sprite descriptor structure
        x      - a new X coordinate
        y      - a new Y coordinate

    RESULT
        None.

    NOTES
        ViewPort is also used in order to specify the physical display.
        If it's not specified, Amiga(tm) chipset display is assumed.
        This is available only on Amiga(tm) architecture.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    OOP_Object *gfxhidd = NULL;
    OOP_Object *display = NULL;
    struct gfxdisplay_data *mdd = NULL;

    if (!sprite || sprite->num > 7)
        return;

    if (vp)
    {
        mdd = GET_VP_DRIVERDATA(vp);
        sprite->x = x + vp->DxOffset;
        sprite->y = y + vp->DyOffset;
    }
    else
    {
        sprite->x = x;
        sprite->y = y;
        for (mdd = CDD(GfxBase)->mdisplay.display_next; mdd; mdd = mdd->display_next)
            if (mdd->display_flags & DF_ExternalPlanar)
                break;
    }

    /* SimpleSprite buffers are live DMA streams, sometimes containing
     * several chained images. Keep the caller's memory rather than turning
     * it into a copied cursor shape. Extended sprites use the HIDD path. */
    if (mdd && (mdd->display_flags & DF_ExternalPlanar) &&
        !(GfxBase->ExtSprites & (1 << sprite->num)) &&
        sprite->posctldata &&
        (GfxBase->SpriteReserved & (1 << sprite->num)))
    {
        UWORD *data = sprite->posctldata;
        struct View *view = GfxBase->ActiView;
        WORD xpos = sprite->x, ypos = sprite->y;
        UWORD stop;

        if (!data || !GfxBase->SimpleSprites ||
            !(GfxBase->SpriteReserved & (1 << sprite->num)))
            return;
        if (vp)
        {
            if (vp->Modes & SUPERHIRES)
                xpos >>= 2;
            else if (vp->Modes & HIRES)
                xpos >>= 1;
            if (vp->Modes & LACE)
                ypos >>= 1;
        }
        xpos += STANDARD_VIEW_X + (view ? view->DxOffset : 0);
        ypos += STANDARD_VIEW_Y + (view ? view->DyOffset : 0);
        stop = ypos + sprite->height;
        Disable();
        data[0] = (ypos << 8) | ((xpos >> 1) & 0xff);
        data[1] = (stop << 8) | (data[1] & SPRITE_ATTACHED) |
                  ((ypos & 0x100) >> 6) | ((stop & 0x100) >> 7) |
                  (xpos & 1);
        GfxBase->SimpleSprites[sprite->num] = sprite;
        Enable();
        return;
    }

    if (vp) {
        gfxhidd = mdd->display_gfxhidd;
        display = mdd->display_obj;
    } else {
        OOP_Class *nativeclass;
        if ((nativeclass = OOP_FindClass(CLID_Hidd_Gfx_AmigaVideo)) != NULL) {
            gfxhidd = (OOP_Object *)OOP_NewObject(nativeclass, NULL, NULL);
            if (gfxhidd)
                OOP_GetAttr(gfxhidd, aHidd_Gfx_DisplayDefault, (IPTR *)&display);
        }
        sprite->x = x;
        sprite->y = y;
    }

    if (gfxhidd) {
        if (sprite->num) {
            OOP_MethodID HiddAmigaGfxBase = OOP_GetMethodID(IID_Hidd_AmigaGfx, 0);
            HIDD_AMIGAGFX_SetSpritePos(gfxhidd, sprite->x, sprite->y, sprite->num);
        } else if (display)
            HIDD_Display_SetCursorPos(display, sprite->x, sprite->y);
        if (!vp) {
            OOP_DisposeObject(gfxhidd);
        }
    }

    AROS_LIBFUNC_EXIT
} /* MoveSprite */
