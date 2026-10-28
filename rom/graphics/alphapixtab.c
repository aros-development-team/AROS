/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Colour updates for bitmaps with an alpha channel that derive their
          colours from a friend without one (AllocBitMap()).
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/gfx.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*
 * Such a bitmap (HIDD_BMF_ALPHA_PIXTAB) has a colormap and pixel table of its
 * own, filled from the friend's colormap with every colour opaque, because the
 * friend's pixel values have no alpha bits. The friend is identified by its
 * colormap object, which all of its ordinary friends share, so a SetRGB32()
 * on the screen reaches every surface allocated from it.
 */

BOOL gfx_AddAlphaPixTab(struct BitMap *bm, OOP_Object *colmap, struct GfxBase *GfxBase)
{
    struct AlphaPixTabNode *n;

    n = AllocMem(sizeof(struct AlphaPixTabNode), MEMF_PUBLIC | MEMF_CLEAR);
    if (!n)
        return FALSE;

    n->bitmap = bm;
    n->colmap = colmap;

    ObtainSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);
    AddTail((struct List *)&PrivGBase(GfxBase)->alphapixtabs, (struct Node *)n);
    ReleaseSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);

    return TRUE;
}

void gfx_ForgetAlphaPixTab(struct BitMap *bm, struct GfxBase *GfxBase)
{
    struct AlphaPixTabNode *n, *next;

    ObtainSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);
    ForeachNodeSafe(&PrivGBase(GfxBase)->alphapixtabs, n, next)
    {
        if (n->bitmap == bm)
        {
            Remove((struct Node *)n);
            FreeMem(n, sizeof(struct AlphaPixTabNode));
        }
    }
    ReleaseSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);
}

/*
 * colors[first..first+count-1] were just set on the bitmap owning colmap.
 * Map them, opaque, into every dependent bitmap's colormap and pixel table.
 */
void gfx_UpdateAlphaPixTabs(OOP_Object *colmap, HIDDT_Color *colors, ULONG first, ULONG count,
                            struct GfxBase *GfxBase)
{
    struct AlphaPixTabNode *n;
    HIDDT_Color *copy = NULL;
    ULONG i;

    if (!colmap || !count || first + count > AROS_PALETTE_SIZE)
        return;

    ObtainSemaphoreShared(&PrivGBase(GfxBase)->alphapixtabs_sema);
    ForeachNode(&PrivGBase(GfxBase)->alphapixtabs, n)
    {
        if (n->colmap != colmap)
            continue;

        if (!copy)
        {
            copy = AllocMem(sizeof(HIDDT_Color) * count, MEMF_ANY);
            if (!copy)
                break;
        }

        for (i = 0; i < count; i++)
        {
            copy[i] = colors[i];
            copy[i].alpha = 0xFFFF;     /* pens are opaque */
        }

        if (HIDD_BM_SetColors(HIDD_BM_OBJ(n->bitmap), copy, first, count))
        {
            for (i = 0; i < count; i++)
                HIDD_BM_PIXTAB(n->bitmap)[first + i] = copy[i].pixval;
        }
    }
    ReleaseSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);

    if (copy)
        FreeMem(copy, sizeof(HIDDT_Color) * count);
}
