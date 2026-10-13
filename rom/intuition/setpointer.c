/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include "intuition_intern.h"
#include <intuition/pointerclass.h>
#include <graphics/sprite.h>
#include <hidd/gfx.h>
#include <proto/intuition.h>

/* Legacy callers often set the same pointer on every animation tick. Keep
 * the existing object when its converted pixels already match the source;
 * comparing the pixels also catches applications that rewrite the source in
 * place before calling SetPointer() again. */
static BOOL unchanged_legacy_pointer(struct Window *window, const UWORD *source,
                                     WORD height, WORD width, WORD xOffset,
                                     WORD yOffset, struct IntuitionBase *IntuitionBase)
{
    struct SharedPointer *shared = NULL;
    OOP_MethodID HiddBitMapBase = GetPrivIBase(IntuitionBase)->ib_HiddBitMapBase;
    UBYTE pixels[16 * 16];
    UWORD row, x;
    ULONG lock;

    if (width <= 0 || width > 16 || height <= 0 || height > 16)
        return FALSE;

    lock = LockIBase(0);
    if (!IW(window)->free_pointer || !IW(window)->pointer || IW(window)->busy ||
        window->PtrWidth != width || window->PtrHeight != height)
        goto changed;

    GetAttr(POINTERA_SharedPointer, IW(window)->pointer, (IPTR *)&shared);
    if (!shared || !shared->sprite || !shared->sprite->es_BitMap ||
        !IS_HIDD_BM(shared->sprite->es_BitMap) ||
        shared->xoffset != xOffset || shared->yoffset != yOffset ||
        shared->sprite->es_SimpleSprite.height != height)
        goto changed;

    HIDD_BM_GetImage(HIDD_BM_OBJ(shared->sprite->es_BitMap), pixels, 16,
                     0, 0, 16, height, vHidd_StdPixFmt_Native);
    source += 2; /* Legacy position/control words precede the image rows. */
    for (row = 0; row < height; row++)
    {
        UWORD plane0 = *source++;
        UWORD plane1 = *source++;

        for (x = 0; x < 16; x++)
        {
            UWORD bit = 0x8000 >> x;
            UBYTE pixel = ((plane0 & bit) != 0) |
                          (((plane1 & bit) != 0) << 1);

            if (pixels[row * 16 + x] != pixel)
                goto changed;
        }
    }

    UnlockIBase(lock);
    return TRUE;

changed:
    UnlockIBase(lock);
    return FALSE;
}

/*****************************************************************************

    NAME */
        AROS_LH6(void, SetPointer,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),
        AROS_LHA(const UWORD   *, pointer, A1),
        AROS_LHA(WORD           , height, D0),
        AROS_LHA(WORD           , width, D1),
        AROS_LHA(WORD           , xOffset, D2),
        AROS_LHA(WORD           , yOffset, D3),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 45, Intuition)

/*  FUNCTION
        Changes the shape of the mouse pointer for a given window.

    INPUTS
        window - Change it for this window
        pointer - The shape of the new pointer as a bitmap with depth 2.
        height - Height of the pointer
        width - Width of the pointer (must be <= 16)
        xOffset, yOffset - The offset of the "hot spot" relative to the
            left, top edge of the bitmap.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ClearPointer()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DEBUG_SETPOINTER(dprintf("SetPointer: window 0x%lx pointer data 0x%lx\n",
                             window, pointer));

    if (window && pointer &&
        !unchanged_legacy_pointer(window, pointer, height, width,
                                  xOffset, yOffset, IntuitionBase))
    {
        Object *ptrobj = MakePointerFromData(IntuitionBase, pointer, xOffset, yOffset, width, height);

        if (ptrobj)
        {
            struct TagItem pointertags[] =
            {
                {WA_Pointer, (IPTR)ptrobj},
                {TAG_DONE                }
            };

            window->Pointer = (UWORD *)pointer;
            window->PtrWidth = width;
            window->PtrHeight = height;

            SetWindowPointerA(window, pointertags);

            IW(window)->free_pointer = TRUE;
        }
    }

    AROS_LIBFUNC_EXIT
} /* SetPointer */
