/*
    Copyright (C) 2011-2026, The AROS Development Team.

    Desc: decorator.library - drawing functions
*/

#define DEBUG 0
#include <aros/debug.h>

#include <intuition/imageclass.h>
#include <graphics/rpattr.h>
#include <libraries/cybergraphics.h>
#include <proto/arossupport.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <proto/exec.h>

#include <hidd/gfx.h>

#include <aros/libcall.h>

#include <math.h>

#include <libraries/decorator.h>
#include "decorator_intern.h"

#define DECOR_USELINEBUFF
//#define DECOR_FAKESHADE
//#define DECOR_NODIRECT
//#define DECOR_NOSHADE

// D = A + B
#define BLIT_MINTERM_COPY        0xC0
// D = A + B + C
#define BLIT_MINTERM_COPYTM      0xE0

struct ShadeData
{
    struct DecorImage     *di;
    struct BitMap     *rpBm;
    UWORD               offy;
    UWORD               fact;
    WORD                startx, starty;
};

struct RectList
{
    ULONG rl_num;
    struct RectList *rl_next;
    struct Rectangle rl_rect;
};

struct layerhookmsg
{
    struct Layer *l;
/*  struct Rectangle rect; (replaced by the next line!) */
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};

/* This function provides a number of ways to blit a DecorImage onto RastPort. Please take great care when modifying it.
 *
 * The number of combinations of arguments is quite high. Please take time to understand it.
 *
 * Arguments:
 * di - a DecorImage that is to be blitted
 * subimageCol, subimageRow - define the initial read offset in source image based on assumption that image contains
 *                            a number of subimages drawn in rows or columns
 * xSrc, ySrc - define additional read offset in the source image subimage
 * destRP - destination RastPort to blit the image to
 * xDest, yDest - coordinates on the destination RastPort to where the imatge will be blitted
 * widthSrc, heightSrc - width/height of region to be read from, if -1 then use the width/height of subimage
 * widthDest, heightDest - width/height of blit on destination RastPort, if -1 then use widthSrc/heightSrc
 *
 */
static void BltScaleDecorImageSubImageRastPort(struct DecorImage * di, ULONG subimageCol, ULONG subimageRow,
        LONG xSrc, LONG ySrc, struct RastPort * destRP, LONG xDest, LONG yDest,
        LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest)
{
    ULONG subimagewidth     = di->w / di->subimagescols;
    ULONG subimageheight    = di->h / di->subimagesrows;
    
    if (subimageCol >= di->subimagescols) return;
    if (subimageRow >= di->subimagesrows) return;

    /* If source size not provided, use subimage size */
    if (widthSrc < 0) widthSrc = (LONG)subimagewidth;
    if (heightSrc < 0) heightSrc = (LONG)subimageheight;

    /* If destination size not provided, use source */
    if (widthDest < 0) widthDest = widthSrc;
    if (heightDest < 0) heightDest = heightSrc;

    /* If source and destination sizes do not match, scale */
    if ((widthSrc != widthDest) || (heightSrc != heightDest))
    {
        /* FIXME: The scaled blitting needs similar optimized code paths as non-scaled */
        ULONG * srcptr = (di->data) + (((subimageheight * subimageRow) + ySrc) * di->w) +
                ((subimagewidth * subimageCol) + xSrc); /* Go to (0,0) of source rect */

        ULONG * scaleddata = ScaleBuffer(srcptr, di->w, widthSrc, heightSrc, widthDest, heightDest);

        D(bug("[Decoration] SCALED %d,%d -> %d,%d!\n", widthSrc, heightSrc, widthDest, heightDest));

        WritePixelArrayAlpha(scaleddata, 0, 0, widthDest * 4, destRP, xDest, yDest, widthDest, heightDest, 0xffffffff);

        FreeVec(scaleddata);
    }
    else /* ((widthSrc != widthDest) || (heightSrc != heightDest)) */
    {
        /* Detect if image can be drawn using blitting instead of alpha draw */
        if ((!di->subimageinbm) || (!(di->subimageinbm[subimageCol + (subimageRow * di->subimagescols)])))
        {
            WritePixelArrayAlpha(di->data, (subimagewidth * subimageCol) + xSrc ,
                (subimageheight * subimageRow) + ySrc, di->w * 4, destRP,
                xDest, yDest, widthSrc, heightSrc, 0xffffffff);
        }
        else
        {
            /* LUT */
            if (di->bitmap != NULL)
            {
                if (di->mask)
                {
                    BltMaskBitMapRastPort(di->bitmap, (subimagewidth * subimageCol) + xSrc ,
                        (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                        widthSrc, heightSrc, BLIT_MINTERM_COPYTM, (PLANEPTR) di->mask);
                }
                else
                {
                    BltBitMapRastPort(di->bitmap, (subimagewidth * subimageCol) + xSrc ,
                        (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                        widthSrc, heightSrc, BLIT_MINTERM_COPY);
                }
            }

            /* Truecolor */
            if (di->bitmap2 != NULL)
            {
                BltBitMapRastPort(di->bitmap2, (subimagewidth * subimageCol) + xSrc ,
                    (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                    widthSrc, heightSrc, BLIT_MINTERM_COPY);
            }
        }
    }
}

/* HELPER WRAPPERS */
static inline void BltDecorImageSubImageRastPort(struct DecorImage * di, ULONG subimageCol, ULONG subimageRow,
        LONG xSrc, LONG ySrc, struct RastPort * destRP, LONG xDest, LONG yDest, LONG widthSrc, LONG heightSrc)
{
    BltScaleDecorImageSubImageRastPort(di, subimageCol, subimageRow, xSrc, ySrc, destRP,
            xDest, yDest, widthSrc, heightSrc, -1, -1);
}

static inline void BltDecorImageSubImageRastPortSimple(struct DecorImage * di, ULONG subimageCol, ULONG subimageRow,
    struct RastPort * destRP, LONG xDest, LONG yDest)
{
    BltDecorImageSubImageRastPort(di, subimageCol, subimageRow, 0, 0, destRP,
            xDest, yDest, -1, -1);
}

static inline void BltScaleDecorImageSubImageRastPortSimple(struct DecorImage * di, ULONG subimageCol, ULONG subimageRow,
    struct RastPort * destRP, LONG xDest, LONG yDest, LONG widthDest, LONG heightDest)
{
    BltScaleDecorImageSubImageRastPort(di, subimageCol, subimageRow, 0, 0, destRP,
            xDest, yDest, -1, -1, widthDest, heightDest);
}
/* HELPER WRAPPERS */

static void DrawTileToImage(struct DecorImage *src, struct DecorImage *dest, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
{

    ULONG   dy, dx;
    LONG    dh, height, dw, width;

    if (src == NULL) return;
    if (dest == NULL) return;

    dh = _sh;
    dy = _dy;
    height = _dh;

    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;
        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;
            DrawPartToImage(src, dest, _sx, _sy, dw, dh, dx, dy);
            dx += dw;
        }
        dy += dh;
    }
}

static void TileImageToImageMenuBar(struct DecorImage *src, struct TileInfo * srcti, struct DecorImage *dest)
{
    UWORD   y, h;

    if (dest == NULL) return;
    if (src == NULL) return;
    if (srcti == NULL) return;
    y = 0;

    h = src->h;

    if ((srcti->TileTop + srcti->TileBottom) > dest->h) return;
    if (srcti->TileRight > dest->w) return;

    DrawTileToImage(src, dest, srcti->TileLeft, y, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileTop, 0, 0, dest->w - srcti->TileRight, srcti->TileTop);
    DrawTileToImage(src, dest, srcti->TileLeft, y + h - srcti->TileBottom, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileBottom, 0, dest->h - srcti->TileBottom, dest->w - srcti->TileRight, srcti->TileBottom);
    DrawTileToImage(src, dest, srcti->TileLeft, y + srcti->TileTop, src->w - srcti->TileLeft - srcti->TileRight, h - srcti->TileBottom - srcti->TileTop, 0, srcti->TileTop + 0, dest->w - srcti->TileRight, dest->h - srcti->TileTop - srcti->TileBottom - 0);


    DrawTileToImage(src, dest, src->w - srcti->TileRight, y, srcti->TileRight, srcti->TileTop, dest->w - srcti->TileRight, 0, srcti->TileRight, srcti->TileTop);
    DrawTileToImage(src, dest, src->w - srcti->TileRight, y + h - srcti->TileBottom, srcti->TileRight, srcti->TileBottom, dest->w - srcti->TileRight , dest->h - srcti->TileBottom, srcti->TileRight, srcti->TileBottom);
    DrawTileToImage(src, dest, src->w - srcti->TileRight, y + srcti->TileTop, srcti->TileRight,  h - srcti->TileBottom - srcti->TileTop, dest->w - srcti->TileRight, srcti->TileTop + 0, srcti->TileRight, dest->h - srcti->TileTop - srcti->TileBottom - 0);

}

static void TileImageToImage(struct DecorImage *src, struct TileInfo * srcti, struct DecorImage *dest)
{
    UWORD   y, h;

    if (dest == NULL) return;
    if (src == NULL) return;
    if (srcti == NULL) return;
    y = 0;

    h = src->h;

    if ((srcti->TileTop + srcti->TileBottom) > dest->h) return;
    if ((srcti->TileLeft + srcti->TileRight) > dest->w) return;

    DrawTileToImage(src, dest, 0, y, srcti->TileLeft, srcti->TileTop, 0 , 0, srcti->TileLeft, srcti->TileTop);
    DrawTileToImage(src, dest, 0, y + h - srcti->TileBottom, srcti->TileLeft, srcti->TileBottom, 0 , dest->h - srcti->TileBottom, srcti->TileLeft, srcti->TileBottom);
    DrawTileToImage(src, dest, src->w - srcti->TileRight, y, srcti->TileRight, srcti->TileTop, dest->w - srcti->TileRight, 0, srcti->TileRight, srcti->TileTop);
    DrawTileToImage(src, dest, src->w - srcti->TileRight, y + h - srcti->TileBottom, srcti->TileRight, srcti->TileBottom, dest->w - srcti->TileRight , dest->h - srcti->TileBottom, srcti->TileRight, srcti->TileBottom);

    DrawTileToImage(src, dest, srcti->TileLeft, y, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileTop, srcti->TileLeft, 0, dest->w - srcti->TileLeft - srcti->TileRight, srcti->TileTop);
    DrawTileToImage(src, dest, srcti->TileLeft, y + h - srcti->TileBottom, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileBottom, srcti->TileLeft, dest->h - srcti->TileBottom, dest->w - srcti->TileLeft - srcti->TileRight, srcti->TileBottom);
    DrawTileToImage(src, dest, 0, y + srcti->TileTop, srcti->TileLeft, h - srcti->TileBottom - srcti->TileTop, 0 , srcti->TileTop + 0, srcti->TileLeft, dest->h - srcti->TileTop - srcti->TileBottom - 0);
    DrawTileToImage(src, dest, src->w - srcti->TileRight, y + srcti->TileTop, srcti->TileRight,  h - srcti->TileBottom - srcti->TileTop, dest->w - srcti->TileRight, srcti->TileTop + 0, srcti->TileRight, dest->h - srcti->TileTop - srcti->TileBottom - 0);
    DrawTileToImage(src, dest, srcti->TileLeft, y + srcti->TileTop, src->w - srcti->TileLeft - srcti->TileRight, h - srcti->TileBottom - srcti->TileTop, srcti->TileLeft, srcti->TileTop + 0, dest->w - srcti->TileLeft - srcti->TileRight, dest->h - srcti->TileTop - srcti->TileBottom - 0);
}

static void  MixImage(struct DecorImage *dst, struct DecorImage *src, struct TileInfo *srcti, UWORD ratio, UWORD w, UWORD h, UWORD dx, UWORD dy)
{
    BOOL    tiled = FALSE;
    int     y;

    if (src == NULL) return;
    if (dst == NULL) return;

    if (srcti) tiled = TRUE;

    for (y = 0; y < h; y++)
    {
        pixop_mix_row(dst->data + dx + (ULONG)(y + dy) * dst->w,
                      src->data + (ULONG)y * src->w, w, ratio, tiled);
    }
}


static void BlurSourceAndMixTexture(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo * textureti, UWORD ratio)
{
    LONG    y;
    int     width, w, height, ah, aw, xpos, ypos;
    BOOL    tiled = FALSE;
    ULONG  *raw;
    LONG    tw, th;

    if (pic == NULL) return;
    if (pic->data == NULL) return;

    tw = pic->w;
    if (textureti) tiled = TRUE;
    th = pic->h;
    raw = pic->data;
    height = th;
    width = tw;

    if (raw)
    {
        /* Rows are blurred in place; keep copies of the unmodified current
           and two previous rows so the blur kernel always reads original
           source data */
        ULONG *rowbufs = AllocVec(tw * sizeof(ULONG) * 3, MEMF_ANY);

        if (rowbufs)
        {
            ULONG *rm2 = rowbufs;
            ULONG *rm1 = rowbufs + tw;
            ULONG *rcur = rowbufs + (tw * 2);

            for (y = 0; y < th; y++)
            {
                const ULONG *prow1, *prow2, *prowb1, *prowb2, *texrow;
                ULONG *rotate;

                CopyMem(&raw[(ULONG)y * tw], rcur, tw * sizeof(ULONG));

                /* Clamp row taps at the image edges */
                prow1 = (y >= 1) ? rm1 : rcur;
                prow2 = (y >= 2) ? rm2 : prow1;
                prowb1 = (y < th - 1) ? &raw[(ULONG)(y + 1) * tw] : rcur;
                prowb2 = (y < th - 2) ? &raw[(ULONG)(y + 2) * tw] : prowb1;
                texrow = tiled ? &texture->data[(ULONG)y * texture->w] : NULL;

                pixop_blur14mix_row(&raw[(ULONG)y * tw], prow2, prow1, rcur,
                                    prowb1, prowb2, texrow, tw);

                rotate = rm2;
                rm2 = rm1;
                rm1 = rcur;
                rcur = rotate;
            }

            FreeVec(rowbufs);
        }
    }
    if (ratio < 100)
    {
        if (texture)
        {
            ypos = 0;
            while (height>0)
            {
                ah = texture->h;
                if (ah > height) ah = height;
                xpos = 0;
                w = width;
                while (w>0)
                {
                    aw = texture->w;
                    if (aw > w) aw = w;
                    MixImage(pic, texture, textureti, 255 - (2.55 * ratio), aw, ah, xpos, ypos);
                    w -= aw;
                    xpos += aw;
                }
                height -= ah;
                ypos += ah;
            }
        }
    }
}

static void RenderBackgroundTiled(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo *textureti,
        UWORD ratio, VOID (*TileImageToImageFunc)(struct DecorImage *src, struct TileInfo * srcti, struct DecorImage *dest))
{
    struct DecorImage *di;

    if (texture)
    {
        di = NewDecorImageContainer(pic->w, pic->h);
        if (di)
        {
            if (textureti)
            {
                TileImageToImageFunc(texture, textureti, di);
                BlurSourceAndMixTexture(pic, di, textureti, ratio);
            }
            else BlurSourceAndMixTexture(pic, texture, textureti, ratio);

            DisposeImageContainer(di);
        }
        else BlurSourceAndMixTexture(pic, texture, textureti, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, NULL, ratio);
}

static void DrawMapTileToRP(struct DecorImage *src, struct RastPort *rp, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
{

    ULONG   dy, dx;
    LONG    dh, height, dw, width;

    if (src == NULL) return;
    if (rp == NULL) return;

    dh = _sh;
    dy = _dy;
    height = _dh;

    if (!src->ok) return;

    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;
        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;

            if (src->mask)
            {
                BltMaskBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, BLIT_MINTERM_COPYTM, (PLANEPTR) src->mask);
            }
            else BltBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, BLIT_MINTERM_COPY);

            dx += dw;
        }
        dy += dh;
    }
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

void DrawPartImageToRP(struct RastPort *rp, struct DecorImage *di, UWORD x, UWORD y, UWORD sx, UWORD sy, UWORD sw, UWORD sh)
{
    if (di->ok)
    {
        if (di->bitmap == NULL)
        {
            WritePixelArray(di->data, sx, sy, di->w*4, rp, x, y, sw, sh, RECTFMT_ARGB);
        }
        else
        {
            BltBitMapRastPort(di->bitmap, sx, sy, rp, x, y, sw, sh, BLIT_MINTERM_COPY);
        }
    }
}

void DrawPartToImage(struct DecorImage *src, struct DecorImage *dest, UWORD sx, UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy)
{
    UWORD   y;

    for (y = 0; y < sh; y++)
    {
        CopyMem(&src->data[sx + (ULONG)(sy + y) * src->w],
                &dest->data[dx + (ULONG)(dy + y) * dest->w],
                (ULONG)sw * sizeof(ULONG));
    }
}

void RenderMenuBackground(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo *textureti, UWORD ratio)
{
    if (texture)
    {
        if (textureti) RenderBackgroundTiled(pic, texture, textureti, ratio, TileImageToImage);
        else BlurSourceAndMixTexture(pic, texture, textureti, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, NULL, ratio);
}

void RenderMenuBarBackground(struct DecorImage *pic, struct DecorImage *texture, struct TileInfo *textureti, UWORD ratio)
{
    if (texture && textureti)
    {

        /* Fill the image with the center tile */
        DrawTileToImage(texture, pic,
                textureti->TileLeft, textureti->TileTop,
                texture->w - textureti->TileLeft - textureti->TileRight, texture->h - textureti->TileBottom - textureti->TileTop,
                0, 0, pic->w, pic->h);

        RenderBackgroundTiled(pic, texture, textureti, ratio, TileImageToImageMenuBar);
    }
}

void WriteAlphaPixelArray(struct DecorImage *src, struct DecorImageLUT8 *dst, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h)
{
    int     y;

    for (y = 0; y < h; y++)
    {
        pixop_extract_alpha_row(dst->data + dx + (ULONG)(dy + y) * dst->w,
                                src->data + sx + (ULONG)(sy + y) * src->w, w);
    }
}

void  SetImageTint(struct DecorImage *dst, UWORD ratio, ULONG argb)
{
    int     y;

    if (dst == NULL) return;

    for (y = 0; y < dst->h; y++)
        pixop_tint_row(dst->data + (ULONG)y * dst->w, dst->w, argb, ratio);
}

/*
 * offx - offset between start of di and place where image should be sample from
 * offy - offset between start of di and place where image should be sample from
 * x, y, w, h - coords in rastport rp
 */
void HorizVertRepeatDecorImage(struct DecorImage *di, ULONG color, UWORD offx, UWORD offy, struct RastPort *rp, UWORD x, UWORD y, WORD w, WORD h)
{
    ULONG   ow, oh, sy, sx, dy, dx;
    LONG    dh, height, dw, width;

    if ((w <= 0) || (h <= 0)) return;

    if (!di->ok)
    {
        FillPixelArray(rp, x, y, w, h, color);
        return;
    }

    ow = di->w;
    oh = di->h;

    sy = offy % oh;
    dh = oh - sy;
    height = h;
    dy = y;
    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;

        sx = offx % ow;
        dw = ow - sx;
        width = w;
        dx = x;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;

            BltDecorImageSubImageRastPort(di, 0, 0, sx, sy, rp, dx, dy, dw, dh);

            dx += dw;
            sx = 0;
            dw = ow;
        }
        dy += dh;
        sy = 0;
        dh = oh;
    }
}

/* NOTE: fill parameter is ignored, previously it was forcing a no-alpha blit, but
   this is already handled in BltDecorImageSubImageRastPort */
/* clipw - if > 0 nothing is drawn beyond this x extent */
/* dh - destination height to which subimage will be scaled to */
LONG WriteTiledImageTitle(BOOL fill, LONG clipw,
    struct RastPort *rp, struct DecorImage *di, LONG sx, LONG sy, LONG sw,
    LONG sh, LONG xp, LONG yp, LONG dw, LONG dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!di->ok) return x;

    if ((sw == 0) || (dw == 0)) return xp;

    if (clipw > 0)
    {
        if (x > clipw) return xp;
        if ((x + w) > clipw) w = clipw - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;

        BltScaleDecorImageSubImageRastPort(di, 0, 0, sx, sy, rp, x, yp, ddw, -1, -1, dh);

        w -= ddw;
        x += ddw;
    }
    return x;
}

/*
 * dh - destination height to scale to, -1 to use subimage height
 */
LONG WriteVerticalScaledTiledImageHorizontal(struct RastPort *rp, struct DecorImage *di, ULONG subimage,
        LONG sx, LONG sw, LONG xp, LONG yp, LONG sh, LONG dw, LONG dh)
{
    LONG w = dw;
    LONG x = xp;
    LONG ddw;

    if (!di->ok) return xp;

    if ((sw == 0) || (dw == 0)) return xp;

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;

        BltScaleDecorImageSubImageRastPort(di, 0, subimage, sx, 0, rp, x, yp, ddw, sh, -1, dh);

        w -= ddw;
        x += ddw;
    }

    return x;
}

LONG WriteTiledImageHorizontal(struct RastPort *rp, struct DecorImage *di, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG dw)
{
    return WriteVerticalScaledTiledImageHorizontal(rp, di, subimage, sx, sw, xp, yp, -1, dw, -1);
}

LONG WriteTiledImageVertical(struct RastPort *rp, struct DecorImage *di, ULONG subimage, LONG sy, LONG sh, LONG xp, LONG yp, LONG dh)
{
    int     h = dh;
    int     y = yp;
    int     ddh;

    if (!di->ok) return y;

    if ((sh == 0) || (dh == 0)) return yp;

    while (h > 0)
    {
        ddh = sh;
        if (h < ddh) ddh = h;

        BltDecorImageSubImageRastPort(di, subimage, 0, 0, sy, rp, xp, y, -1, ddh);

        h -= ddh;
        y += ddh;
    }
    return y;
}

struct myrgb
{
    int red,green,blue;
};

void FillMemoryBufferRGBGradient(UBYTE * buf, LONG pen, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle)
{
    /* The basic idea of this algorithm is to calc the intersection between the
     * diagonal of the rectangle (xs,ys) with dimension (xw,yw) a with the line starting
     * at (x,y) (every pixel inside the rectangle) and angle angle with direction vector (vx,vy).
     *
     * Having the intersection point we then know the color of the pixel.
     *
     * TODO: Turn the algorithm into a incremental one
     *       Remove the use of floating point variables */
    double rad = angle*M_PI/180;
    double cosarc = cos(rad);
    double sinarc = sin(rad);

    struct myrgb startRGB,endRGB;

    int diffR, diffG, diffB;

    int r,t; /* some helper variables to short the code */
    int l,y,c,x;
    int y1; /* The intersection point */
    int incr_y1; /* increment of y1 */
    int xs,ys,xw,yw;
    int xadd,ystart,yadd;
//    double vx = -cosarc;
//    double vy = sinarc;
    int vx = (int)(-cosarc*0xff);
    int vy = (int)(sinarc*0xff);
    
    int width = xb - xt + 1;
    int height = yb - yt + 1;

    if (buf == NULL) return;

    startRGB.red = (start_rgb >> 16) & 0xff;
    startRGB.green = (start_rgb >> 8) & 0xff;
    startRGB.blue = start_rgb & 0xff;

    endRGB.red = (end_rgb >> 16) & 0xff;
    endRGB.green = (end_rgb >> 8) & 0xff;
    endRGB.blue = end_rgb & 0xff;

    diffR = endRGB.red - startRGB.red;
    diffG = endRGB.green - startRGB.green;
    diffB = endRGB.blue - startRGB.blue;

    /* Normalize the angle */
    if (angle < 0) angle = 360 - ((-angle)%360);
    if (angle >= 0) angle = angle % 360;

    if (angle <= 90 || (angle > 180 && angle <= 270))
    {
        /* The to be intersected diagonal goes from the top left edge to the bottom right edge */
        xs = 0;
        ys = 0;
        xw = width;
        yw = height;
    } else
    {
        /* The to be intersected diagonal goes from the bottom left edge to the top right edge */
        xs = 0;
        ys = height;
        xw = width;
        yw = -height;
    }
                
    if (angle > 90 && angle <= 270)
    {
        /* for these angle we have y1 = height - y1. Instead of
         *
         *  y1 = height - (-vy*(yw*  xs -xw*  ys)         + yw*(vy*  x -vx*  y))        /(-yw*vx + xw*vy);
         *
         * we can write
         *
         *  y1 =          (-vy*(yw*(-xs)-xw*(-ys+height)) + yw*(vy*(-x)-vx*(-y+height)))/(-yw*vx + xw*vy);
         *
         * so height - y1 can be expressed with the normal formular adapting some parameters.
         *
         * Note that if one would exchanging startRGB/endRGB the values would only work
         * for linear color gradients */
        xadd = -1;
        yadd = -1;
        ystart = height;

        xs = -xs;
        ys = -ys + height;
    }
    else
    {
        xadd = 1;
        yadd = 1;
        ystart = 0;
    }

    r = -vy*(yw*xs-xw*ys);
    t = -yw*vx + xw*vy;

    /* The formular as shown above is
     *
     *   y1 = ((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));
     *
     * We see that only yw*(vy*x-vx*y) changes during the loop.
     *
     * We write
     *
     *   Current Pixel: y1(x,y) = (r + yw*(vy*x-vx*y))/t = r/t + yw*(vy*x-vx*y)/t
     *   Next Pixel:    y1(x+xadd,y) = (r + vw*(vy*(x+xadd)-vx*y))/t
     *
     *   t*(y1(x+xadd,y) - y1(x,y)) = yw*(vy*(x+xadd)-vx*y) - yw*(vy*x-vx*y) = yw*vy*xadd; */

    incr_y1 = yw*vy*xadd;
    UBYTE *bufptr = buf;
    for (l = 0, y = ystart + ((yp - yt)* yadd); l < h; l++, y+=yadd)
    {
        /* Calculate initial y1 accu, can be brought out of the loop as well (x=0). It's probably a
         * a good idea to add here also a value of (t-1)/2 to ensure the correct rounding
         * This (and for r) is also a place were actually a overflow can happen |yw|=16 |y|=16. So for
         * vx nothing is left, currently 9 bits are used for vx or vy */
        int y1_mul_t_accu = r - yw*vx*y;


       
        for (c = 0, x = ((xp - xt) * xadd); c < w; c++, x+=xadd)
        {
            int red,green,blue;

            /* Calculate the intersection of two lines, this is not the fastet way to do but
             * it is intuitive. Note: very slow! Will be optimzed later (remove FFP usage
             * and making it incremental)...update: it's now incremental and no FFP is used
             * but it probably can be optimized more by removing some more of the divisions and
             * further specialize the stuff here (use of three accus). */
            /*      y1 = (int)((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));*/
            y1 = y1_mul_t_accu / t;

            red = startRGB.red + (int)(diffR*y1/height);
            green = startRGB.green + (int)(diffG*y1/height);
            blue = startRGB.blue + (int)(diffB*y1/height);
            /* By using full 32 bits this can be made faster as well */
            *bufptr++ = red;
            *bufptr++ = green;
            *bufptr++ = blue;

            y1_mul_t_accu += incr_y1;
        }
    }

}
    
void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle, LONG dx, LONG dy)
{
    UBYTE * buf = NULL;
    
    if ((w <= 0) || (h <= 0)) return;

    /* By bringing building the gradient array in the same format as the RastPort BitMap a call
      to WritePixelArray() can be made also faster */
    buf = AllocVec(1 * yb * 3, 0);
    
    FillMemoryBufferRGBGradient(buf, pen, xt, yt, xb, yb, xp, yp, 1, yb, start_rgb, end_rgb, angle);

    HorizRepeatBuffer(buf, dy, pen, tc, rp, xp, yp, w, h);

    FreeVec(buf);
}

void HorizRepeatBuffer(UBYTE * buf, LONG offy, LONG pen, BOOL tc, struct RastPort *rp, LONG x, LONG y, LONG w, LONG h)
{
    UBYTE * bufblit = NULL;
    ULONG yi;
    ULONG idxs;

    if ((w <= 0) || (h <= 0)) return;
    if (!tc)
    {
        if (pen != -1) SetAPen(rp, pen); else SetAPen(rp, 2);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        return;
    }

    /* By bringing building the gradient array in the same format as the RastPort BitMap a call
       to WritePixelArray() can be made also faster */
    bufblit = AllocVec(w * h * 3, MEMF_ANY);
    if (!bufblit)
        return;
    
    /* Copy one column buffer into blit buffer; each row is a single RGB
       value repeated, so write the first pixel and then double the filled
       region until the row is full */
    for (yi = 0; yi < h; yi++)
    {
        UBYTE *row = bufblit + ((ULONG)yi * w * 3);
        LONG rowlen = w * 3;
        LONG filled, copylen;

        idxs = (offy + yi) * 3; /* source index */
        row[0] = buf[idxs + 0];
        row[1] = buf[idxs + 1];
        row[2] = buf[idxs + 2];

        for (filled = 3; filled < rowlen; filled += copylen)
        {
            copylen = filled;
            if (copylen > rowlen - filled) copylen = rowlen - filled;
            CopyMem(row, row + filled, copylen);
        }
    }
    
    WritePixelArray(bufblit, 0, 0, w * 3, rp, x, y, w, h, RECTFMT_RGB);

    FreeVec(bufblit);
}

void TileMapToBitmap(struct DecorImage *src, struct TileInfo *srcti, struct BitMap *map, UWORD dw, UWORD dh)
{
    UWORD   y, h;

    if (map == NULL) return;
    if (src == NULL) return;
    if (srcti == NULL) return;
    y = 0;

    h = src->h;

    if ((srcti->TileTop + srcti->TileBottom) > dh) return;
    if ((srcti->TileLeft + srcti->TileRight) > dw) return;

    struct RastPort *dest = CreateRastPort();

    if (dest != NULL)
    {
        dest->BitMap = map;

        DrawMapTileToRP(src, dest, 0, y, srcti->TileLeft, srcti->TileTop, 0 , 0, srcti->TileLeft, srcti->TileTop);
        DrawMapTileToRP(src, dest, 0, y + h - srcti->TileBottom, srcti->TileLeft, srcti->TileBottom, 0 , dh - srcti->TileBottom, srcti->TileLeft, srcti->TileBottom);
        DrawMapTileToRP(src, dest, src->w - srcti->TileRight, y, srcti->TileRight, srcti->TileTop, dw - srcti->TileRight, 0, srcti->TileRight, srcti->TileTop);
        DrawMapTileToRP(src, dest, src->w - srcti->TileRight, y + h - srcti->TileBottom, srcti->TileRight, srcti->TileBottom, dw - srcti->TileRight , dh - srcti->TileBottom, srcti->TileRight, srcti->TileBottom);

        DrawMapTileToRP(src, dest, srcti->TileLeft, y, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileTop, srcti->TileLeft, 0, dw - srcti->TileLeft - srcti->TileRight, srcti->TileTop);
        DrawMapTileToRP(src, dest, srcti->TileLeft, y + h - srcti->TileBottom, src->w - srcti->TileLeft - srcti->TileRight, srcti->TileBottom, srcti->TileLeft, dh - srcti->TileBottom, dw - srcti->TileLeft - srcti->TileRight, srcti->TileBottom);
        DrawMapTileToRP(src, dest, 0, y + srcti->TileTop, srcti->TileLeft, h - srcti->TileBottom - srcti->TileTop, 0 , srcti->TileTop + 0, srcti->TileLeft, dh - srcti->TileTop - srcti->TileBottom - 0);
        DrawMapTileToRP(src, dest, src->w - srcti->TileRight, y + srcti->TileTop, srcti->TileRight,  h - srcti->TileBottom - srcti->TileTop, dw - srcti->TileRight, srcti->TileTop + 0, srcti->TileRight, dh - srcti->TileTop - srcti->TileBottom - 0);
        DrawMapTileToRP(src, dest, srcti->TileLeft, y + srcti->TileTop, src->w - srcti->TileLeft - srcti->TileRight, h - srcti->TileBottom - srcti->TileTop, srcti->TileLeft, srcti->TileTop + 0, dw - srcti->TileLeft - srcti->TileRight, dh - srcti->TileTop - srcti->TileBottom - 0);
        FreeRastPort(dest);
    }
}

struct DecorImage *GetImageFromRP(struct RastPort *rp, UWORD x, UWORD y, UWORD w, UWORD h)
{
    struct DecorImage *di;

    di = NewDecorImageContainer(w, h);
    if (di)
    {
        ReadPixelArray(di->data, 0, 0, w*4, rp, x, y, w, h, RECTFMT_ARGB);
    }
    return di;
}

void PutImageToRP(struct RastPort *rp, struct DecorImage *di, UWORD x, UWORD y) {

    if (di)
    {
        if (di->data) WritePixelArray(di->data, 0, 0, di->w*4, rp, x, y, di->w, di->h, RECTFMT_ARGB);
        DisposeImageContainer(di);
    }
}

ULONG CalcShade(ULONG base, UWORD fact)
{
    int     c0, c1, c2, c3;

    c0 = GET_ARGB_A(base);
    c1 = GET_ARGB_R(base);
    c2 = GET_ARGB_G(base);
    c3 = GET_ARGB_B(base);
#if !defined(DECOR_NOSHADE)
    c0 *= fact;
    c1 *= fact;
    c2 *= fact;
    c3 *= fact;
    c0 = c0 >> 8;
    c1 = c1 >> 8;
    c2 = c2 >> 8;
    c3 = c3 >> 8;
#endif
    if (c0 > 255) c0 = 255;
    if (c1 > 255) c1 = 255;
    if (c2 > 255) c2 = 255;
    if (c3 > 255) c3 = 255;

    return (ULONG)SET_ARGB(c0, c1, c2, c3);
}

AROS_UFH3(void, RectShadeFunc,
    AROS_UFHA(struct Hook *        , h,      A0),
    AROS_UFHA(struct RastPort *    , rp,     A2),
    AROS_UFHA(struct layerhookmsg *, msg,    A1))
{
    AROS_USERFUNC_INIT

    struct ShadeData *data = h->h_Data;

#if defined(DECOR_USELINEBUFF)
    ULONG               *outline = NULL;
    ULONG               linesize = 0;
#endif
    UWORD               startx, starty, width = 1 + msg->MaxX - msg->MinX;
    UWORD               height = 1 + msg->MaxY - msg->MinY;
    WORD                src_offset_x = 0;
    WORD                src_offset_y = 0;
    struct RastPort     *dstRp;
    ULONG               color;

    HIDDT_Color         col;
    APTR                bm_handle = NULL;
    int                 px, py;
#if !defined(DECOR_FAKESHADE)
    int                 x = 0, y;

#endif
    D(
       bug("[Decoration] %s: data @ 0x%p\n", __func__, data);
       bug("[Decoration] %s:      offy = %d, fact = %d, di @ 0x%p\n", __func__, data->offy, data->fact, data->di);
       bug("[Decoration] %s: Msg  Area = %d,%d -> %d,%d\n", __func__, msg->MinX, msg->MinY, msg->MaxX, msg->MaxY);
       bug("[Decoration] %s:      Offset = %d,%d\n", __func__, msg->OffsetX, msg->OffsetY);
    )

    if (data->rpBm == rp->BitMap)
    {
        startx = msg->MinX;
        starty = msg->MinY;
        src_offset_x = msg->OffsetX;
        src_offset_y = msg->OffsetY;
    }
    else
    {
        struct ClipRect * CR;

        // Hidden cliprect..
        startx = msg->MinX;
        starty = msg->MinY;

        for (CR=rp->Layer->ClipRect;CR;CR=CR->Next)
        {
            if (CR->BitMap == rp->BitMap)
            {
                src_offset_x = startx + (CR->bounds.MinX - rp->Layer->bounds.MinX) - ALIGN_OFFSET(CR->bounds.MinX);
                src_offset_y = starty + (CR->bounds.MinY - rp->Layer->bounds.MinY);
            }
        }
    }

    D(
       bug("[Decoration] %s: Offset = %d,%d\n", __func__, src_offset_x, src_offset_y);
    )

#if defined(DECOR_USELINEBUFF)
    if (width > 1)
        linesize = width << 2;
    else if (height > 1)
    {
#if !defined(DECOR_FAKESHADE)
        x = src_offset_x % data->di->w;
#endif
        linesize = height << 2;
    }

    if (linesize)
    {
        outline = AllocMem(linesize, MEMF_ANY);
    }

    if (!outline)
#endif
    {
#if !defined(DECOR_NODIRECT)
        bm_handle = LockBitMapTags(rp->BitMap,
                    TAG_END);
#endif
        dstRp = rp;
    }
    else
    {
        dstRp = CloneRastPort(rp);
        dstRp->Layer = NULL;
    }

    for (py = starty; py < (starty + height); py++)
    {
#if !defined(DECOR_FAKESHADE)
        y = (src_offset_y + py - starty) % data->di->h;
#endif

#if defined(DECOR_USELINEBUFF)
        if (width == 1 && outline)
        {
#if !defined(DECOR_FAKESHADE)
            color = CalcShade(data->di->data[(y * data->di->w) + x], data->fact);
#else
            color = SET_ARGB(00, 0xFF, 0x00, 0x70);
#endif
            outline[py - starty] = color;

            continue;
        }

#endif
#if defined(DECOR_USELINEBUFF) && !defined(DECOR_FAKESHADE)
        if (outline && (width > 1))
        {
            /* Shade the whole scanline via the row kernel, split into
               segments where the source image wraps around */
            LONG niw = data->di->w;
            const ULONG *srow = data->di->data + ((ULONG)y * niw);
            LONG remaining = width, oidx = 0;
            LONG sx = ((src_offset_x % niw) + niw) % niw;

            while (remaining > 0)
            {
                LONG seg = niw - sx;
                if (seg > remaining) seg = remaining;
                pixop_shade_row(&outline[oidx], &srow[sx], seg, data->fact);
                oidx += seg;
                remaining -= seg;
                sx = 0;
            }
        }
        else
#endif
        for (px = startx; px < (startx + width); px++)
        {
#if !defined(DECOR_FAKESHADE)
            x = (src_offset_x + px - startx) % data->di->w;

            color = CalcShade(data->di->data[(y * data->di->w) + x], data->fact);
#else
            color = SET_ARGB(00, 0xFF, 0x00, 0x70);
#endif

#if defined(DECOR_USELINEBUFF)
            if (outline)
            {
                outline[px - startx] = color;
            }
            else
#endif
                if (bm_handle)
                {
                    col.alpha = (HIDDT_ColComp) GET_ARGB_A(color) << 8;
                    col.red = (HIDDT_ColComp) GET_ARGB_R(color) << 8;
                    col.green = (HIDDT_ColComp) GET_ARGB_G(color) << 8;
                    col.blue = (HIDDT_ColComp) GET_ARGB_B(color) << 8;

                    HIDD_BM_PutPixel(HIDD_BM_OBJ(dstRp->BitMap), px, py, HIDD_BM_MapColor(HIDD_BM_OBJ(dstRp->BitMap), &col));
                }
                else
                {
                    WriteRGBPixel(dstRp,
                        px,
                        py,
#if AROS_BIG_ENDIAN
                        color);
#else
                        SET_ARGB(GET_ARCH_A(color), GET_ARCH_R(color), GET_ARCH_G(color), GET_ARCH_B(color)));
#endif
                }
        }
#if defined(DECOR_USELINEBUFF)
        if (outline && (width > 1))
        {
            WritePixelArray(outline,
                0, 0,
                linesize,
                dstRp,
                startx,
                starty,
                width, 1,
                RECTFMT_ARGB);
        }
#endif
    }

#if defined(DECOR_USELINEBUFF)
    if (outline)
    {
        if (width == 1)
        {
            WritePixelArray(outline,
                0, 0,
                sizeof(ULONG),
                dstRp,
                startx,
                starty,
                1, height,
                RECTFMT_ARGB);
        }
        FreeMem(outline, linesize);
    }
    else
#endif
        if (bm_handle)
        {
            struct RectList bm_rectlist;
            struct TagItem bm_ultags[3] =
            {
                {UBMI_REALLYUNLOCK, TRUE                },
                {UBMI_UPDATERECTS,  (IPTR)&bm_rectlist  },
                {TAG_DONE, 0                            }
            };

            bm_rectlist.rl_num = 1;
            bm_rectlist.rl_next = (struct RectList *)0;
            bm_rectlist.rl_rect.MinX = msg->MinX;
            bm_rectlist.rl_rect.MinY = msg->MinY;
            bm_rectlist.rl_rect.MaxX = msg->MaxX;
            bm_rectlist.rl_rect.MaxY = msg->MaxY;

            UnLockBitMapTagList(bm_handle, bm_ultags);
        }

    if (dstRp != rp)
        FreeRastPort(dstRp);

    AROS_USERFUNC_EXIT
}

void ShadeLine(LONG pen, BOOL tc, BOOL usegradients, struct RastPort *rp, struct DecorImage *di, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    ULONG   color;

    if ((x1 < x0) || (y1 < y0)) return;
    if (!tc)
    {
        SetAPen(rp, pen);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
        return;
    }
    if (usegradients)
    {
        D(bug("[Decoration] %s: GRADIENT > %d,%d -> %d,%d\n", __func__, x0, y0, x1, y1);)

        color = CalcShade(basecolor, fact);

        SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, color, TAG_DONE);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
    else if (di->ok)
    {
        struct ShadeData shadeParams;
        struct Hook      shadeHook;
        struct Rectangle shadeRect;

        shadeRect.MinX = x0;
        shadeRect.MaxX = x1;
        shadeRect.MinY = y0;
        shadeRect.MaxY = y1;

        D(bug("[Decoration] %s: SHADE > %d,%d -> %d,%d\n", __func__, x0, y0, x1, y1);)

        shadeParams.di = di;
        shadeParams.rpBm = rp->BitMap;
        shadeParams.startx = x0,
        shadeParams.starty = y0;
        shadeParams.offy = _offy;
        shadeParams.fact = fact;

        shadeHook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(RectShadeFunc);
        shadeHook.h_Data = &shadeParams;

        DoHookClipRects(&shadeHook, rp, &shadeRect);
    }
    else
    {
        D(bug("[Decoration] %s: DRAW > %d,%d -> %d,%d\n", __func__, x0, y0, x1, y1);)
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
}

void DrawScaledStatefulGadgetImageToRP(struct RastPort *rp, struct DecorImage *di, ULONG state, UWORD xp, UWORD yp,
        WORD scaledwidth, WORD scaledheight)
{

    UWORD subimagecol = 0;
    UWORD subimagerow = 0;
    
    if (di->ok)
    {
        switch(state)
        {
            case IDS_NORMAL:
                break;
            case IDS_SELECTED:
                subimagecol = 1;
                break;
            case IDS_INACTIVENORMAL:
                subimagecol = 2;
                break;
        }

        BltScaleDecorImageSubImageRastPortSimple(di, subimagecol, subimagerow, rp, xp, yp, scaledwidth, scaledheight);
    }
}

void DrawStatefulGadgetImageToRP(struct RastPort *rp, struct DecorImage *di, ULONG state, UWORD xp, UWORD yp)
{
    DrawScaledStatefulGadgetImageToRP(rp, di, state, xp, yp, -1, -1);
}

/******************************************************************************/
/* AROS Library Function Wrappers                                             */
/******************************************************************************/

AROS_LH8(void, DDrawPartImageToRP,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(UWORD, x, D0),
    AROS_LHA(UWORD, y, D1),
    AROS_LHA(UWORD, sx, D2),
    AROS_LHA(UWORD, sy, D3),
    AROS_LHA(UWORD, sw, D4),
    AROS_LHA(UWORD, sh, D5),
    struct Library *, DecoratorBase, 15, Decorator)
{
    AROS_LIBFUNC_INIT
    DrawPartImageToRP(rp, di, x, y, sx, sy, sw, sh);
    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, DDrawPartToImage,
    AROS_LHA(struct DecorImage *, src, A0),
    AROS_LHA(struct DecorImage *, dest, A1),
    AROS_LHA(UWORD, sx, D0),
    AROS_LHA(UWORD, sy, D1),
    AROS_LHA(UWORD, sw, D2),
    AROS_LHA(UWORD, sh, D3),
    AROS_LHA(UWORD, dx, D4),
    AROS_LHA(UWORD, dy, D5),
    struct Library *, DecoratorBase, 16, Decorator)
{
    AROS_LIBFUNC_INIT
    DrawPartToImage(src, dest, sx, sy, sw, sh, dx, dy);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, DDrawStatefulGadgetImageToRP,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, state, D0),
    AROS_LHA(UWORD, xp, D1),
    AROS_LHA(UWORD, yp, D2),
    struct Library *, DecoratorBase, 17, Decorator)
{
    AROS_LIBFUNC_INIT
    DrawStatefulGadgetImageToRP(rp, di, state, xp, yp);
    AROS_LIBFUNC_EXIT
}

AROS_LH7(void, DDrawScaledStatefulGadgetImageToRP,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, state, D0),
    AROS_LHA(UWORD, xp, D1),
    AROS_LHA(UWORD, yp, D2),
    AROS_LHA(WORD, scaledwidth, D3),
    AROS_LHA(WORD, scaledheight, D4),
    struct Library *, DecoratorBase, 18, Decorator)
{
    AROS_LIBFUNC_INIT
    DrawScaledStatefulGadgetImageToRP(rp, di, state, xp, yp, scaledwidth, scaledheight);
    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, DHorizVertRepeatDecorImage,
    AROS_LHA(struct DecorImage *, di, A0),
    AROS_LHA(ULONG, color, D0),
    AROS_LHA(UWORD, offx, D1),
    AROS_LHA(UWORD, offy, D2),
    AROS_LHA(struct RastPort *, rp, A1),
    AROS_LHA(UWORD, x, D3),
    AROS_LHA(UWORD, y, D4),
    AROS_LHA(WORD, w, D5),
    AROS_LHA(WORD, h, D6),
    struct Library *, DecoratorBase, 19, Decorator)
{
    AROS_LIBFUNC_INIT
    HorizVertRepeatDecorImage(di, color, offx, offy, rp, x, y, w, h);
    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, DHorizRepeatBuffer,
    AROS_LHA(UBYTE *, buf, A0),
    AROS_LHA(LONG, offy, D0),
    AROS_LHA(LONG, pen, D1),
    AROS_LHA(BOOL, tc, D2),
    AROS_LHA(struct RastPort *, rp, A1),
    AROS_LHA(LONG, x, D3),
    AROS_LHA(LONG, y, D4),
    AROS_LHA(LONG, w, D5),
    AROS_LHA(LONG, h, D6),
    struct Library *, DecoratorBase, 20, Decorator)
{
    AROS_LIBFUNC_INIT
    HorizRepeatBuffer(buf, offy, pen, tc, rp, x, y, w, h);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, DFillPixelArrayGradient,
    AROS_LHA(LONG, pen, D0),
    AROS_LHA(BOOL, tc, D1),
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct GradientSpec *, spec, A1),
    struct Library *, DecoratorBase, 21, Decorator)
{
    AROS_LIBFUNC_INIT
    FillPixelArrayGradient(pen, tc, rp, spec->gs_xt, spec->gs_yt, spec->gs_xb, spec->gs_yb,
        spec->gs_xp, spec->gs_yp, spec->gs_w, spec->gs_h,
        spec->gs_StartRGB, spec->gs_EndRGB, spec->gs_Angle, spec->gs_dx, spec->gs_dy);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, DRenderMenuBackground,
    AROS_LHA(struct DecorImage *, pic, A0),
    AROS_LHA(struct DecorImage *, texture, A1),
    AROS_LHA(struct TileInfo *, textureti, A2),
    AROS_LHA(UWORD, ratio, D0),
    struct Library *, DecoratorBase, 22, Decorator)
{
    AROS_LIBFUNC_INIT
    RenderMenuBackground(pic, texture, textureti, ratio);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, DRenderMenuBarBackground,
    AROS_LHA(struct DecorImage *, pic, A0),
    AROS_LHA(struct DecorImage *, texture, A1),
    AROS_LHA(struct TileInfo *, textureti, A2),
    AROS_LHA(UWORD, ratio, D0),
    struct Library *, DecoratorBase, 23, Decorator)
{
    AROS_LIBFUNC_INIT
    RenderMenuBarBackground(pic, texture, textureti, ratio);
    AROS_LIBFUNC_EXIT
}

AROS_LH9(void, DShadeLine,
    AROS_LHA(LONG, pen, D0),
    AROS_LHA(BOOL, tc, D1),
    AROS_LHA(BOOL, usegradients, D2),
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, basecolor, D3),
    AROS_LHA(UWORD, fact, D4),
    AROS_LHA(UWORD, offy, D5),
    AROS_LHA(struct Rectangle *, bounds, A2),
    struct Library *, DecoratorBase, 24, Decorator)
{
    AROS_LIBFUNC_INIT
    ShadeLine(pen, tc, usegradients, rp, di, basecolor, fact, offy,
              bounds->MinX, bounds->MinY, bounds->MaxX, bounds->MaxY);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, DSetImageTint,
    AROS_LHA(struct DecorImage *, dst, A0),
    AROS_LHA(UWORD, ratio, D0),
    AROS_LHA(ULONG, argb, D1),
    struct Library *, DecoratorBase, 25, Decorator)
{
    AROS_LIBFUNC_INIT
    SetImageTint(dst, ratio, argb);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(void, DTileMapToBitmap,
    AROS_LHA(struct DecorImage *, src, A0),
    AROS_LHA(struct TileInfo *, srcti, A1),
    AROS_LHA(struct BitMap *, map, A2),
    AROS_LHA(UWORD, dw, D0),
    AROS_LHA(UWORD, dh, D1),
    struct Library *, DecoratorBase, 26, Decorator)
{
    AROS_LIBFUNC_INIT
    TileMapToBitmap(src, srcti, map, dw, dh);
    AROS_LIBFUNC_EXIT
}

AROS_LH8(void, DWriteAlphaPixelArray,
    AROS_LHA(struct DecorImage *, src, A0),
    AROS_LHA(struct DecorImageLUT8 *, dst, A1),
    AROS_LHA(LONG, sx, D0),
    AROS_LHA(LONG, sy, D1),
    AROS_LHA(LONG, dx, D2),
    AROS_LHA(LONG, dy, D3),
    AROS_LHA(LONG, w, D4),
    AROS_LHA(LONG, h, D5),
    struct Library *, DecoratorBase, 27, Decorator)
{
    AROS_LIBFUNC_INIT
    WriteAlphaPixelArray(src, dst, sx, sy, dx, dy, w, h);
    AROS_LIBFUNC_EXIT
}

AROS_LH12(LONG, DWriteTiledImageTitle,
    AROS_LHA(BOOL, fill, D0),
    AROS_LHA(struct Window *, win, A0),
    AROS_LHA(struct RastPort *, rp, A1),
    AROS_LHA(struct DecorImage *, di, A2),
    AROS_LHA(LONG, sx, D1),
    AROS_LHA(LONG, sy, D2),
    AROS_LHA(LONG, sw, D3),
    AROS_LHA(LONG, sh, D4),
    AROS_LHA(LONG, xp, D5),
    AROS_LHA(LONG, yp, D6),
    AROS_LHA(LONG, dw, D7),
    AROS_LHA(LONG, dh, A3),
    struct Library *, DecoratorBase, 28, Decorator)
{
    AROS_LIBFUNC_INIT
    return WriteTiledImageTitle(fill, win ? win->Width : 0, rp, di, sx, sy, sw, sh, xp, yp, dw, dh);
    AROS_LIBFUNC_EXIT
}

AROS_LH8(LONG, DWriteTiledImageVertical,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, subimage, D0),
    AROS_LHA(LONG, sy, D1),
    AROS_LHA(LONG, sh, D2),
    AROS_LHA(LONG, xp, D3),
    AROS_LHA(LONG, yp, D4),
    AROS_LHA(LONG, dh, D5),
    struct Library *, DecoratorBase, 29, Decorator)
{
    AROS_LIBFUNC_INIT
    return WriteTiledImageVertical(rp, di, subimage, sy, sh, xp, yp, dh);
    AROS_LIBFUNC_EXIT
}

AROS_LH8(LONG, DWriteTiledImageHorizontal,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, subimage, D0),
    AROS_LHA(LONG, sx, D1),
    AROS_LHA(LONG, sw, D2),
    AROS_LHA(LONG, xp, D3),
    AROS_LHA(LONG, yp, D4),
    AROS_LHA(LONG, dw, D5),
    struct Library *, DecoratorBase, 30, Decorator)
{
    AROS_LIBFUNC_INIT
    return WriteTiledImageHorizontal(rp, di, subimage, sx, sw, xp, yp, dw);
    AROS_LIBFUNC_EXIT
}

AROS_LH10(LONG, DWriteVerticalScaledTiledImageHorizontal,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(ULONG, subimage, D0),
    AROS_LHA(LONG, sx, D1),
    AROS_LHA(LONG, sw, D2),
    AROS_LHA(LONG, xp, D3),
    AROS_LHA(LONG, yp, D4),
    AROS_LHA(LONG, sh, D5),
    AROS_LHA(LONG, dw, D6),
    AROS_LHA(LONG, dh, D7),
    struct Library *, DecoratorBase, 31, Decorator)
{
    AROS_LIBFUNC_INIT
    return WriteVerticalScaledTiledImageHorizontal(rp, di, subimage, sx, sw, xp, yp, sh, dw, dh);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, DPutImageToRP,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(struct DecorImage *, di, A1),
    AROS_LHA(UWORD, x, D0),
    AROS_LHA(UWORD, y, D1),
    struct Library *, DecoratorBase, 32, Decorator)
{
    AROS_LIBFUNC_INIT
    PutImageToRP(rp, di, x, y);
    AROS_LIBFUNC_EXIT
}

AROS_LH5(struct DecorImage *, DGetImageFromRP,
    AROS_LHA(struct RastPort *, rp, A0),
    AROS_LHA(UWORD, x, D0),
    AROS_LHA(UWORD, y, D1),
    AROS_LHA(UWORD, w, D2),
    AROS_LHA(UWORD, h, D3),
    struct Library *, DecoratorBase, 33, Decorator)
{
    AROS_LIBFUNC_INIT
    return GetImageFromRP(rp, x, y, w, h);
    AROS_LIBFUNC_EXIT
}

AROS_LH13(void, DFillMemoryBufferRGBGradient,
    AROS_LHA(UBYTE *, buf, A0),
    AROS_LHA(LONG, pen, D0),
    AROS_LHA(LONG, xt, D1),
    AROS_LHA(LONG, yt, D2),
    AROS_LHA(LONG, xb, D3),
    AROS_LHA(LONG, yb, D4),
    AROS_LHA(LONG, xp, D5),
    AROS_LHA(LONG, yp, D6),
    AROS_LHA(LONG, w, D7),
    AROS_LHA(LONG, h, A1),
    AROS_LHA(ULONG, start_rgb, A2),
    AROS_LHA(ULONG, end_rgb, A3),
    AROS_LHA(LONG, angle, A4),
    struct Library *, DecoratorBase, 34, Decorator)
{
    AROS_LIBFUNC_INIT
    FillMemoryBufferRGBGradient(buf, pen, xt, yt, xb, yb, xp, yp, w, h, start_rgb, end_rgb, angle);
    AROS_LIBFUNC_EXIT
}
