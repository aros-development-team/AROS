/*
    Copyright � 2011-2014, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#include <intuition/imageclass.h>
#include <graphics/rpattr.h>
#include <libraries/cybergraphics.h>
#include <proto/arossupport.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <proto/exec.h>

#include <hidd/graphics.h>

#include <math.h>

#include "drawfuncs.h"

#if AROS_BIG_ENDIAN
#define GET_A(rgb) ((rgb >> 24) & 0xff)
#define GET_R(rgb) ((rgb >> 16) & 0xff)
#define GET_G(rgb) ((rgb >> 8) & 0xff)
#define GET_B(rgb) (rgb & 0xff)
#define SET_ARGB(a, r, g, b) (a << 24 | r << 16 | g << 8 | b)
#else
#define GET_A(rgb) (rgb & 0xff)
#define GET_R(rgb) ((rgb >> 8) & 0xff)
#define GET_G(rgb) ((rgb >> 16) & 0xff)
#define GET_B(rgb) ((rgb >> 24) & 0xff)
#define SET_ARGB(a, r, g, b) (b << 24 | g << 16 | r << 8 | a)
#endif

struct ShadeData
{
    struct NewImage     *ni;
    UWORD               fact;
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

/* This function provides a number of ways to blit a NewImage onto RastPort. Please take great care when modifying it.
 *
 * The number of combinations of arguments is quite high. Please take time to understand it.
 *
 * Arguments:
 * ni - a NewImage that is to be blitted
 * subimageCol, subimageRow - define the initial read offset in source image based on assumption that image contains
 *                            a number of subimages drawn in rows or columns
 * xSrc, ySrc - define additional read offset in the source image subimage
 * destRP - destination RastPort to blit the image to
 * xDest, yDest - coordinates on the destination RastPort to where the imatge will be blitted
 * widthSrc, heightSrc - width/height of region to be read from, if -1 then use the width/height of subimage
 * widthDest, heightDest - width/height of blit on destination RastPort, if -1 then use widthSrc/heightSrc
 *
 */
static void BltScaleNewImageSubImageRastPort(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
        LONG xSrc, LONG ySrc, struct RastPort * destRP, LONG xDest, LONG yDest,
        LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest)
{
    ULONG subimagewidth     = ni->w / ni->subimagescols;
    ULONG subimageheight    = ni->h / ni->subimagesrows;
    
    if (subimageCol >= ni->subimagescols) return;
    if (subimageRow >= ni->subimagesrows) return;

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
        ULONG * srcptr = (ni->data) + (((subimageheight * subimageRow) + ySrc) * ni->w) +
                ((subimagewidth * subimageCol) + xSrc); /* Go to (0,0) of source rect */

        ULONG * scaleddata = ScaleBuffer(srcptr, ni->w, widthSrc, heightSrc, widthDest, heightDest);

        D(bug("[Decoration] SCALED %d,%d -> %d,%d!\n", widthSrc, heightSrc, widthDest, heightDest));

        WritePixelArrayAlpha(scaleddata, 0, 0, widthDest * 4, destRP, xDest, yDest, widthDest, heightDest, 0xffffffff);

        FreeVec(scaleddata);
    }
    else /* ((widthSrc != widthDest) || (heightSrc != heightDest)) */
    {
        /* Detect if image can be drawn using blitting instead of alpha draw */
        if ((!ni->subimageinbm) || (!(ni->subimageinbm[subimageCol + (subimageRow * ni->subimagescols)])))
        {
            WritePixelArrayAlpha(ni->data, (subimagewidth * subimageCol) + xSrc ,
                (subimageheight * subimageRow) + ySrc, ni->w * 4, destRP,
                xDest, yDest, widthSrc, heightSrc, 0xffffffff);
        }
        else
        {
            /* LUT */
            if (ni->bitmap != NULL)
            {
                if (ni->mask)
                {
                    BltMaskBitMapRastPort(ni->bitmap, (subimagewidth * subimageCol) + xSrc ,
                        (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                        widthSrc, heightSrc, 0xe0, (PLANEPTR) ni->mask);
                }
                else
                {
                    BltBitMapRastPort(ni->bitmap, (subimagewidth * subimageCol) + xSrc ,
                        (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                        widthSrc, heightSrc, 0xc0);
                }
            }

            /* Truecolor */
            if (ni->bitmap2 != NULL)
            {
                BltBitMapRastPort(ni->bitmap2, (subimagewidth * subimageCol) + xSrc ,
                    (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                    widthSrc, heightSrc, 0xc0);
            }
        }
    }
}

/* HELPER WRAPPERS */
static inline void BltNewImageSubImageRastPort(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
        LONG xSrc, LONG ySrc, struct RastPort * destRP, LONG xDest, LONG yDest, LONG widthSrc, LONG heightSrc)
{
    BltScaleNewImageSubImageRastPort(ni, subimageCol, subimageRow, xSrc, ySrc, destRP,
            xDest, yDest, widthSrc, heightSrc, -1, -1);
}

static inline void BltNewImageSubImageRastPortSimple(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
    struct RastPort * destRP, LONG xDest, LONG yDest)
{
    BltNewImageSubImageRastPort(ni, subimageCol, subimageRow, 0, 0, destRP,
            xDest, yDest, -1, -1);
}

static inline void BltScaleNewImageSubImageRastPortSimple(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
    struct RastPort * destRP, LONG xDest, LONG yDest, LONG widthDest, LONG heightDest)
{
    BltScaleNewImageSubImageRastPort(ni, subimageCol, subimageRow, 0, 0, destRP,
            xDest, yDest, -1, -1, widthDest, heightDest);
}
/* HELPER WRAPPERS */

static void DrawTileToImage(struct NewImage *src, struct NewImage *dest, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
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

static void TileImageToImageMenuBar(struct NewImage *src, struct TileInfo * srcti, struct NewImage *dest)
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

static void TileImageToImage(struct NewImage *src, struct TileInfo * srcti, struct NewImage *dest)
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

static void  MixImage(struct NewImage *dst, struct NewImage *src, struct TileInfo *srcti, UWORD ratio, UWORD w, UWORD h, UWORD dx, UWORD dy)
{
    ULONG  *s, *d;
    ULONG   rgba, rgb;
    UWORD   r, g, b;
    UBYTE   as, rs, gs, bs, rd, gd, bd;
    BOOL    tiled = FALSE;
    int     x, y;

    if (src == NULL) return;
    if (dst == NULL) return;

    s = src->data;
    d = dst->data;

    if (srcti) tiled = TRUE;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            rgba = s[x+y*src->w];
            as = GET_A(rgba);
            rs = GET_R(rgba);
            gs = GET_G(rgba);
            bs = GET_B(rgba);
            rgb = d[x+dx + (y+dy) * dst->w];

            rd = GET_R(rgb);
            gd = GET_G(rgb);
            bd = GET_B(rgb);

            r = ((rs * ratio) >> 8) + ((rd * (255 - ratio)) >> 8);
            g = ((gs * ratio) >> 8) + ((gd * (255 - ratio)) >> 8);
            b = ((bs * ratio) >> 8) + ((bd * (255 - ratio)) >> 8);

            if (tiled) {
                r = ((r * as) >> 8) + ((rd * (255 - as)) >> 8);
                g = ((g * as) >> 8) + ((gd * (255 - as)) >> 8);
                b = ((b * as) >> 8) + ((bd * (255 - as)) >> 8);
            }

            r = r & 0xff;
            g = g & 0xff;
            b = b & 0xff;

            d[x+dx + (y+dy) * dst->w] = SET_ARGB(as, r, g, b);
        }
    }
}


static void BlurSourceAndMixTexture(struct NewImage *pic, struct NewImage *texture, struct TileInfo * textureti, UWORD ratio)
{
    int     x, y, ytw, t1, t2, b1, b2, l1, l2, r1, r2;
    UWORD   red, green, blue, alpha= 0, rs, gs, bs, as;
    ULONG   rgb, argb;
    int     width, w, height, ah, aw, xpos, ypos;
    BOOL    tiled = FALSE;
    ULONG  *raw, tw, th;

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
        for (y = 0; y < th; y++)
        {
            t1 = tw;
            t2 = tw+tw;
            b1 = tw;
            b2 = tw+tw;
            if (y == 0) t1 = t2 = 0;
            else if (y == 1) t2 = t1;

            if (y == (th-1)) b1 = b2 = 0;
            else if (y == (th-2)) b2 = b1;

            ytw = y*tw;

            for (x = 0; x < tw; x++)
            {
                r1 = 1;
                r2 = 2;
                l1 = 1;
                l2 = 2;

                if (x == 0) l1 = r1 = 0;
                else if (x == 1) l2 = l1;

                if (x == (tw-1)) r1 = r2 = 0;
                else if (x == (tw-2)) r2 = r1;

                rgb = raw[x+ytw];
                red = GET_R(rgb);
                green = GET_G(rgb);
                blue = GET_B(rgb);

                rgb = raw[x+ytw-t2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l1-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+r2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1-l1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                red = red/14;
                green = green/14;
                blue = blue/14;
                alpha = 255;

                if (tiled)
                {
                    argb = raw[x+ytw];
                    as = 255 - GET_A(texture->data[x + y * texture->w]);
                    rs = GET_R(argb);
                    gs = GET_G(argb);
                    bs = GET_B(argb);

                    red = ((rs * as) >> 8) + ((red * (255 - as)) >> 8);
                    green = ((gs * as) >> 8) + ((green * (255 - as)) >> 8);
                    blue = ((bs * as) >> 8) + ((blue * (255 - as)) >> 8);

                    raw[x+ytw] = SET_ARGB(as, red, green, blue);

                }
                else
                {
                    raw[x+ytw] = SET_ARGB(alpha, red, green, blue);
                }
            }
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

static void RenderBackgroundTiled(struct NewImage *pic, struct NewImage *texture, struct TileInfo *textureti,
        UWORD ratio, VOID (*TileImageToImageFunc)(struct NewImage *src, struct TileInfo * srcti, struct NewImage *dest))
{
    struct NewImage *ni;

    if (texture)
    {
        ni = NewImageContainer(pic->w, pic->h);
        if (ni)
        {
            if (textureti)
            {
                TileImageToImageFunc(texture, textureti, ni);
                BlurSourceAndMixTexture(pic, ni, textureti, ratio);
            }
            else BlurSourceAndMixTexture(pic, texture, textureti, ratio);

            DisposeImageContainer(ni);
        }
        else BlurSourceAndMixTexture(pic, texture, textureti, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, NULL, ratio);
}

static void DrawMapTileToRP(struct NewImage *src, struct RastPort *rp, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
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
                BltMaskBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, 0xe0, (PLANEPTR) src->mask);  
            }
            else BltBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, 0xc0);

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

void DrawPartImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y, UWORD sx, UWORD sy, UWORD sw, UWORD sh)
{
    if (ni->ok)
    {
        if (ni->bitmap == NULL)
        {
            WritePixelArray(ni->data, sx, sy, ni->w*4, rp, x, y, sw, sh, RECTFMT_ARGB);
        }
        else
        {
            BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, y, sw, sh, 0xc0);
        }
    }
}

void DrawPartToImage(struct NewImage *src, struct NewImage *dest, UWORD sx, UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy)
{
    UWORD   x, y;

    for (y = 0; y < sh; y++)
    {
        for (x = 0; x < sw; x++)
        {
            dest->data[dx  + x + (dy + y) * dest->w] = src->data[sx + x + (sy + y) * src->w];
        }
    }
}

void RenderMenuBackground(struct NewImage *pic, struct NewImage *texture, struct TileInfo *textureti, UWORD ratio)
{
    if (texture)
    {
        if (textureti) RenderBackgroundTiled(pic, texture, textureti, ratio, TileImageToImage);
        else BlurSourceAndMixTexture(pic, texture, textureti, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, NULL, ratio);
}

void RenderMenuBarBackground(struct NewImage *pic, struct NewImage *texture, struct TileInfo *textureti, UWORD ratio)
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

void WriteAlphaPixelArray(struct NewImage *src, struct NewLUT8Image *dst, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h)
{
    ULONG  *s = src->data;
    ULONG   argb;
    UBYTE  *d = dst->data;
    int     x, y;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            argb = s[sx + x + (sy + y) * src->w];
            d[dx + x + (dy + y) * dst->w] = GET_A(argb);
        }
    }
}

void  SetImageTint(struct NewImage *dst, UWORD ratio, ULONG argb)
{

    ULONG  *d;
    ULONG   rgb;
    UWORD   r, g, b, w, h;
    UBYTE   rs, gs, bs, rd, gd, bd;
    int     x, y;

    if (dst == NULL) return;
   
    d = dst->data;

    w = dst->w;
    h = dst->h;
   
    rs = (argb >> 16) & 0xff;
    gs = (argb >> 8) & 0xff;
    bs = argb & 0xff;
   
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            rgb = d[x + y * w];
            rd = GET_R(rgb);
            gd = GET_G(rgb);
            bd = GET_B(rgb);
            r = ((rs * ratio) >> 8) + ((rd * (255 - ratio)) >> 8);
            g = ((gs * ratio) >> 8) + ((gd * (255 - ratio)) >> 8);
            b = ((bs * ratio) >> 8) + ((bd * (255 - ratio)) >> 8);

            r = r & 0xff;
            g = g & 0xff;
            b = b & 0xff;

            d[x + y * w] = SET_ARGB(255, r, g, b);
        }
    }
}

/* 
 * offx - offset between start of ni and place where image should be sample from
 * offy - offset between start of ni and place where image should be sample from
 * x, y, w, h - coords in rastport rp
 */
void HorizVertRepeatNewImage(struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, struct RastPort *rp, UWORD x, UWORD y, WORD w, WORD h)
{
    ULONG   ow, oh, sy, sx, dy, dx;
    LONG    dh, height, dw, width;

    if ((w <= 0) || (h <= 0)) return;

    if (!ni->ok)
    {
        FillPixelArray(rp, x, y, w, h, color);
        return;
    }

    ow = ni->w;
    oh = ni->h;

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

            BltNewImageSubImageRastPort(ni, 0, 0, sx, sy, rp, dx, dy, dw, dh);

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
   this is already handled in BltNewImageSubImageRastPort */
/* dh - destination height to which subimage will be scaled to */
LONG WriteTiledImageTitle(BOOL fill, struct Window *win,
    struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw,
    LONG sh, LONG xp, LONG yp, LONG dw, LONG dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return x;

    if ((sw == 0) || (dw == 0)) return xp;

    if (win)
    {
        if (x > win->Width) return xp;
        if ((x + w) > win->Width) w = win->Width - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;

        BltScaleNewImageSubImageRastPort(ni, 0, 0, sx, sy, rp, x, yp, ddw, -1, -1, dh);

        w -= ddw;
        x += ddw;
    }
    return x;
}

/*
 * dh - destination height to scale to, -1 to use subimage height
 */
LONG WriteVerticalScaledTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, ULONG subimage,
        LONG sx, LONG sw, LONG xp, LONG yp, LONG sh, LONG dw, LONG dh)
{
    LONG w = dw;
    LONG x = xp;
    LONG ddw;

    if (!ni->ok) return xp;

    if ((sw == 0) || (dw == 0)) return xp;

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;

        BltScaleNewImageSubImageRastPort(ni, 0, subimage, sx, 0, rp, x, yp, ddw, sh, -1, dh);

        w -= ddw;
        x += ddw;
    }

    return x;
}

LONG WriteTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG dw)
{
    return WriteVerticalScaledTiledImageHorizontal(rp, ni, subimage, sx, sw, xp, yp, -1, dw, -1);
}

LONG WriteTiledImageVertical(struct RastPort *rp, struct NewImage *ni, ULONG subimage, LONG sy, LONG sh, LONG xp, LONG yp, LONG dh)
{
    int     h = dh;
    int     y = yp;
    int     ddh;

    if (!ni->ok) return y;

    if ((sh == 0) || (dh == 0)) return yp;

    while (h > 0)
    {
        ddh = sh;
        if (h < ddh) ddh = h;

        BltNewImageSubImageRastPort(ni, subimage, 0, 0, sy, rp, xp, y, -1, ddh);

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
     * 	 y1 = ((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));
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
            /*	    y1 = (int)((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));*/
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
    ULONG xi, yi;
    ULONG idxd;
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
    
    /* Copy one column buffer into blit buffer */
    for (yi = 0; yi < h; yi++)
    {
        idxs = (offy + yi) * 3; /* source index */
        idxd = yi * 3 * w; /* dest index */
        for (xi = 0; xi < w; xi++)
        {
            /* Copy RGB pixel */
            bufblit[idxd++] = buf[idxs + 0];
            bufblit[idxd++] = buf[idxs + 1];
            bufblit[idxd++] = buf[idxs + 2];
        }
    }
    
    WritePixelArray(bufblit, 0, 0, w * 3, rp, x, y, w, h, RECTFMT_RGB);

    FreeVec(bufblit);
}

void TileMapToBitmap(struct NewImage *src, struct TileInfo *srcti, struct BitMap *map, UWORD dw, UWORD dh)
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

struct NewImage *GetImageFromRP(struct RastPort *rp, UWORD x, UWORD y, UWORD w, UWORD h)
{
    struct NewImage *ni;

    ni = NewImageContainer(w, h);
    if (ni)
    {
        ReadPixelArray(ni->data, 0, 0, w*4, rp, x, y, w, h, RECTFMT_ARGB);
    }
    return ni;
}

void PutImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y) {

    if (ni)
    {
        if (ni->data) WritePixelArray(ni->data, 0, 0, ni->w*4, rp, x, y, ni->w, ni->h, RECTFMT_ARGB);
        DisposeImageContainer(ni);
    }
}

ULONG CalcShade(ULONG base, UWORD fact)
{
    int     c0, c1, c2, c3;

    c0 = GET_A(base);
    c1 = GET_R(base);
    c2 = GET_G(base);
    c3 = GET_B(base);
    c0 *= fact;
    c1 *= fact;
    c2 *= fact;
    c3 *= fact;
    c0 = c0 >> 8;
    c1 = c1 >> 8;
    c2 = c2 >> 8;
    c3 = c3 >> 8;

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
    ULONG       color;
    HIDDT_Color col;
    APTR        bm_handle;
    int         px, py, x, y;

    bm_handle = LockBitMapTags(rp->BitMap,
                    TAG_END);

    for (px = msg->MinX; px < (msg->MaxX + 1); px++)
    {
        x = (px - rp->Layer->bounds.MinX) % data->ni->w;
        for (py = msg->MinY; py < (msg->MaxY + 1); py++)
        {
            y = (py - rp->Layer->bounds.MinY) % data->ni->h;

            color = CalcShade(data->ni->data[(y * data->ni->w) + x], data->fact);

            if (bm_handle)
            {
                col.alpha = (HIDDT_ColComp) GET_A(color) << 8;
                col.red = (HIDDT_ColComp) GET_R(color) << 8;
                col.green = (HIDDT_ColComp) GET_G(color) << 8;
                col.blue = (HIDDT_ColComp) GET_B(color) << 8;

                HIDD_BM_PutPixel(HIDD_BM_OBJ(rp->BitMap), px, py, HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col));
            }
            else
            {
                WriteRGBPixel(rp, px + msg->OffsetX - msg->MinX, py + msg->OffsetY - msg->MinY, color);
            }
        }
    }

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

    AROS_USERFUNC_EXIT
}

void ShadeLine(LONG pen, BOOL tc, BOOL usegradients, struct RastPort *rp, struct NewImage *ni, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
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
        color = CalcShade(basecolor, fact);
        SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, color, TAG_DONE);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
    else if (ni->ok)
    {
        struct ShadeData shadeParams;
        struct Hook      shadeHook;
        struct Rectangle shadeRect;

        shadeRect.MinX = x0;
        shadeRect.MaxX = x1;
        shadeRect.MinY = y0;
        shadeRect.MaxY = y1;

        shadeParams.ni = ni;
        shadeParams.fact = fact;

        shadeHook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(RectShadeFunc);
        shadeHook.h_Data = &shadeParams;

        DoHookClipRects(&shadeHook, rp, &shadeRect);
    }
    else
    {
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
}

void DrawScaledStatefulGadgetImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp,
        WORD scaledwidth, WORD scaledheight)
{

    UWORD subimagecol = 0;
    UWORD subimagerow = 0;
    
    if (ni->ok)
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

        BltScaleNewImageSubImageRastPortSimple(ni, subimagecol, subimagerow, rp, xp, yp, scaledwidth, scaledheight);
    }
}

void DrawStatefulGadgetImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp)
{
    DrawScaledStatefulGadgetImageToRP(rp, ni, state, xp, yp, -1, -1);
}
