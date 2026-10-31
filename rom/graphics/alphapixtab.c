/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Colour updates for bitmaps that derive their colours from a friend
          in another pixel layout (AllocBitMap()).
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/gfx.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*
 * Such a bitmap (HIDD_BMF_OWN_PIXTAB) has a colormap and pixel table of its
 * own, filled from the friend's colormap with every colour opaque, because the
 * friend's pixel values are in another layout (or have no alpha bits). The
 * source is identified by its colormap object, which all of its ordinary
 * friends share, so a SetRGB32() on the screen reaches every bitmap allocated
 * from it.
 */

/*
 * The colormap a friend's colours come from: its own, or for a bitmap that
 * derives its colours itself, the one those come from, so that a chain of
 * friends follows the screen.
 */
OOP_Object *gfx_PixTabSource(struct BitMap *friend_bitmap, struct GfxBase *GfxBase)
{
    OOP_Object *colmap = HIDD_BM_COLMAP(friend_bitmap);
    struct AlphaPixTabNode *n;

    if (!(HIDD_BM_FLAGS(friend_bitmap) & HIDD_BMF_OWN_PIXTAB))
        return colmap;

    ObtainSemaphoreShared(&PrivGBase(GfxBase)->alphapixtabs_sema);
    ForeachNode(&PrivGBase(GfxBase)->alphapixtabs, n)
    {
        if (n->bitmap == friend_bitmap)
        {
            colmap = n->colmap;
            break;
        }
    }
    ReleaseSemaphore(&PrivGBase(GfxBase)->alphapixtabs_sema);

    return colmap;
}

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
