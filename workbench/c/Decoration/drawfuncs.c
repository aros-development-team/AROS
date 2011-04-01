/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <intuition/imageclass.h>
#include <graphics/rpattr.h>
#include <libraries/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/exec.h>
#include <math.h>

#include "drawfuncs.h"

#if AROS_BIG_ENDIAN
#define GET_A(rgb) ((rgb >> 24) & 0xff)
#define GET_R(rgb) ((rgb >> 16) & 0xff)
#define GET_G(rgb) ((rgb >> 8) & 0xff)
#define GET_B(rgb) (rgb & 0xff)
#define SET_ARGB(a, r, g, b) a << 24 | r << 16 | g << 8 | b
#else
#define GET_A(rgb) (rgb & 0xff)
#define GET_R(rgb) ((rgb >> 8) & 0xff)
#define GET_G(rgb) ((rgb >> 16) & 0xff)
#define GET_B(rgb) ((rgb >> 24) & 0xff)
#define SET_ARGB(a, r, g, b) b << 24 | g << 16 | r << 8 | a
#endif

static void BltNewImageSubImageRastPort(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
    LONG xSrc, LONG ySrc, struct RastPort * destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize)
{
    ULONG subimagewidth     = ni->w / ni->subimagescols;
    ULONG subimageheight    = ni->h / ni->subimagesrows;
    
    if (subimageCol >= ni->subimagescols) return;
    if (subimageRow >= ni->subimagesrows) return;

    /* If destination size not provided, use subimage size */
    if (xSize < 0) xSize = (LONG)subimagewidth;
    if (ySize < 0) ySize = (LONG)subimageheight;

    /* Detect if image can be drawn using blitting instead of alpha draw */
    if (!(ni->subimageinbm[subimageCol + (subimageRow * subimageCol)]))
    {
        WritePixelArrayAlpha(ni->data, (subimagewidth * subimageCol) + xSrc , 
            (subimageheight * subimageRow) + ySrc, ni->w * 4, destRP,
            xDest, yDest, xSize, ySize, 0xffffffff);
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
                    xSize, ySize, 0xe0, (PLANEPTR) ni->mask);  
            }
            else
            {
                BltBitMapRastPort(ni->bitmap, (subimagewidth * subimageCol) + xSrc ,
                    (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                    xSize, ySize, 0xc0);
            }
        }
        
        /* Truecolor */
        if (ni->bitmap2 != NULL)
        {
            BltBitMapRastPort(ni->bitmap2, (subimagewidth * subimageCol) + xSrc ,
                (subimageheight * subimageRow) + ySrc, destRP, xDest, yDest,
                xSize, ySize, 0xc0);
        }
    }

}

static void BltNewImageSubImageRastPortSimple(struct NewImage * ni, ULONG subimageCol, ULONG subimageRow,
    struct RastPort * destRP, LONG xDest, LONG yDest)
{
    BltNewImageSubImageRastPort(ni, subimageCol, subimageRow, 0, 0, destRP, 
        xDest, yDest, -1, -1);
}

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

static void TileImageToImage(struct NewImage *src, struct NewImage *dest)
{
    ULONG  *s, *d;
    UWORD   y, h;

    if (dest == NULL) return;
    if (src == NULL) return;
    y = 0;

    h = src->h;

    s = src->data;
    d = dest->data;
    if (src->istiled == FALSE) return;
    dest->istiled = TRUE;

    if ((src->tile_top + src->tile_bottom) > dest->h) return;
    if ((src->tile_left + src->tile_right) > dest->w) return;

    DrawTileToImage(src, dest, 0, y, src->tile_left, src->tile_top, 0 , 0, src->tile_left, src->tile_top);
    DrawTileToImage(src, dest, 0, y + h - src->tile_bottom, src->tile_left, src->tile_bottom, 0 , dest->h - src->tile_bottom, src->tile_left, src->tile_bottom);
    DrawTileToImage(src, dest, src->w - src->tile_right, y, src->tile_right, src->tile_top, dest->w - src->tile_right, 0, src->tile_right, src->tile_top);
    DrawTileToImage(src, dest, src->w - src->tile_right, y + h - src->tile_bottom, src->tile_right, src->tile_bottom, dest->w - src->tile_right , dest->h - src->tile_bottom, src->tile_right, src->tile_bottom);

    DrawTileToImage(src, dest, src->tile_left, y, src->w - src->tile_left - src->tile_right, src->tile_top, src->tile_left, 0, dest->w - src->tile_left - src->tile_right, src->tile_top);
    DrawTileToImage(src, dest, src->tile_left, y + h - src->tile_bottom, src->w - src->tile_left - src->tile_right, src->tile_bottom, src->tile_left, dest->h - src->tile_bottom, dest->w - src->tile_left - src->tile_right, src->tile_bottom);
    DrawTileToImage(src, dest, 0, y + src->tile_top, src->tile_left, h - src->tile_bottom - src->tile_top, 0 , src->tile_top + 0, src->tile_left, dest->h - src->tile_top - src->tile_bottom - 0);
    DrawTileToImage(src, dest, src->w - src->tile_right, y + src->tile_top, src->tile_right,  h - src->tile_bottom - src->tile_top, dest->w - src->tile_right, src->tile_top + 0, src->tile_right, dest->h - src->tile_top - src->tile_bottom - 0);
    DrawTileToImage(src, dest, src->tile_left, y + src->tile_top, src->w - src->tile_left - src->tile_right, h - src->tile_bottom - src->tile_top, src->tile_left, src->tile_top + 0, dest->w - src->tile_left - src->tile_right, dest->h - src->tile_top - src->tile_bottom - 0);
}

static void  MixImage(struct NewImage *dst, struct NewImage *src, UWORD ratio, UWORD w, UWORD h, UWORD dx, UWORD dy)
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

    if (src) if (src->istiled) tiled = TRUE;

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


static void BlurSourceAndMixTexture(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
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
    if (texture) if (texture->istiled) tiled = TRUE;
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
                    MixImage(pic, texture, 255 - (2.55 * ratio), aw, ah, xpos, ypos);
                    w -= aw;
                    xpos += aw;
                }
                height -= ah;
                ypos += ah;
            }
        }
    }
}

static void RenderBackgroundTiled(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
{
    struct NewImage *ni;

    if (texture)
    {
        ni = NewImageContainer(pic->w, pic->h);
        if (ni)
        {
            if (texture->istiled)
            {
                TileImageToImage(texture, ni);
                BlurSourceAndMixTexture(pic, ni, ratio);
            }
            else BlurSourceAndMixTexture(pic, texture, ratio);

            DisposeImageContainer(ni);
        }
        else BlurSourceAndMixTexture(pic, texture, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, ratio);
}

static int WriteTiledImageNoAlpha(struct Window *win, struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
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
        if (ni->bitmap == NULL)
        {
            WritePixelArray(ni->data, sx , sy, ni->w*4, rp, x, yp, ddw, dh, RECTFMT_ARGB);
        }
        else
        {
            BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
        }
        w -= ddw;
        x += ddw;
    }
    return x;
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

void RenderBackground(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
{
    if (texture)
    {
        if (texture->istiled) RenderBackgroundTiled(pic, texture, ratio); else BlurSourceAndMixTexture(pic, texture, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, ratio);
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
void DrawTileToRP(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h)
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

    sy = (y + offy ) % oh;
    dh = oh - sy;
    height = h;
    dy = y;
    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;

        sx = (x + offx) % ow;
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

LONG WriteTiledImageTitle(BOOL fill, struct Window *win, struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return x;

    if (!fill) return WriteTiledImageNoAlpha(win, rp, ni, sx, sy, sw, sh, xp, yp, dw, dh);

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
        if (ni->bitmap == NULL)
        {
            if (fill) WritePixelArrayAlpha(ni->data, sx, sy, ni->w*4, rp, x, yp, ddw, dh, 0xffffffff); //RECTFMT_ARGB);
            else WritePixelArray(ni->data, sx, sy, ni->w*4, rp, x, yp, ddw, dh, RECTFMT_ARGB);

        }
        else
        {
            if (fill)
            {
                if (ni->mask)
                {
                    BltMaskBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xe0, (PLANEPTR) ni->mask);  
                }
                else BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
            }
            else
            {
                BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
            }
        }
        w -= ddw;
        x += ddw;
    }
    return x;
}

LONG WriteTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, ULONG subimage, LONG sx, LONG sw, LONG xp, LONG yp, LONG dw)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return xp;

    if ((sw == 0) || (dw == 0)) return xp;

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;

        BltNewImageSubImageRastPort(ni, 0, subimage, sx, 0, rp, x, yp, ddw, -1);

        w -= ddw;
        x += ddw;
    }
    return x;
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
    *       Remove the use of floating point variables
    */
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
        * for linear color gradients
 */
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
    *   t*(y1(x+xadd,y) - y1(x,y)) = yw*(vy*(x+xadd)-vx*y) - yw*(vy*x-vx*y) = yw*vy*xadd;
    *
    */

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
    
void FillPixelArrayGradientDelta(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle, LONG dx, LONG dy)
{
    UBYTE * buf = NULL;
    
    if ((w <= 0) || (h <= 0)) return;
    if (!tc)
    {
        if (pen != -1) SetAPen(rp, pen); else SetAPen(rp, 2);
        RectFill(rp, xp, yp, xp + w - 1, yp + h - 1);
        return;
    }

	/* By bringing building the gradient array in the same format as the RastPort BitMap a call
        to WritePixelArray() can be made also faster */
    buf = AllocVec(w * h * 3, 0);
    FillMemoryBufferRGBGradient(buf, pen, xt, yt, xb, yb, xp, yp, w, h, start_rgb, end_rgb, angle);

    WritePixelArray(buf, 0, 0, w * 3, rp, dx, dy, w, h, RECTFMT_RGB);

    FreeVec(buf);
}

void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle)
{
    FillPixelArrayGradientDelta(pen, tc, rp, xt, yt, xb, yb, xp, yp, w, h, start_rgb, end_rgb, angle, xp, yp);
}

void TileMapToBitmap(struct NewImage *src, struct BitMap *map, UWORD dw, UWORD dh)
{
    UWORD   y, h;

    if (map == NULL) return;
    if (src == NULL) return;
    y = 0;

    h = src->h;

    if (src->istiled == FALSE) return;

    if ((src->tile_top + src->tile_bottom) > dh) return;
    if ((src->tile_left + src->tile_right) > dw) return;

    struct RastPort *dest = CreateRastPort();

    if (dest != NULL)
    {
        dest->BitMap = map;

        DrawMapTileToRP(src, dest, 0, y, src->tile_left, src->tile_top, 0 , 0, src->tile_left, src->tile_top);
        DrawMapTileToRP(src, dest, 0, y + h - src->tile_bottom, src->tile_left, src->tile_bottom, 0 , dh - src->tile_bottom, src->tile_left, src->tile_bottom);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y, src->tile_right, src->tile_top, dw - src->tile_right, 0, src->tile_right, src->tile_top);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y + h - src->tile_bottom, src->tile_right, src->tile_bottom, dw - src->tile_right , dh - src->tile_bottom, src->tile_right, src->tile_bottom);

        DrawMapTileToRP(src, dest, src->tile_left, y, src->w - src->tile_left - src->tile_right, src->tile_top, src->tile_left, 0, dw - src->tile_left - src->tile_right, src->tile_top);
        DrawMapTileToRP(src, dest, src->tile_left, y + h - src->tile_bottom, src->w - src->tile_left - src->tile_right, src->tile_bottom, src->tile_left, dh - src->tile_bottom, dw - src->tile_left - src->tile_right, src->tile_bottom);
        DrawMapTileToRP(src, dest, 0, y + src->tile_top, src->tile_left, h - src->tile_bottom - src->tile_top, 0 , src->tile_top + 0, src->tile_left, dh - src->tile_top - src->tile_bottom - 0);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y + src->tile_top, src->tile_right,  h - src->tile_bottom - src->tile_top, dw - src->tile_right, src->tile_top + 0, src->tile_right, dh - src->tile_top - src->tile_bottom - 0);
        DrawMapTileToRP(src, dest, src->tile_left, y + src->tile_top, src->w - src->tile_left - src->tile_right, h - src->tile_bottom - src->tile_top, src->tile_left, src->tile_top + 0, dw - src->tile_left - src->tile_right, dh - src->tile_top - src->tile_bottom - 0);
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

void ShadeLine(LONG pen, BOOL tc, BOOL usegradients, struct RastPort *rp, struct NewImage *ni, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    int px, py, x, y;
    ULONG   c;
    int     c0, c1, c2, c3;
    UWORD   offy = 0;

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
        c = basecolor;
        c3 = (c >> 24) & 0xff;
        c2 = (c >> 16) & 0xff;
        c1 = (c >> 8) & 0xff;
        c0 = c & 0xff;
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
        c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
        SetRPAttrs(rp, RPTAG_FgColor, c, TAG_DONE);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
    else if (ni->ok)
    {
        if (x0 == x1)
        {
            x = x0 % ni->w; 
            for (py = y0; py < y1; py++)
            {
                y = (py - offy) % ni->h;
                c = ni->data[x + y * ni->w];
                c0 = (c >> 24) & 0xff;
                c1 = (c >> 16) & 0xff;
                c2 = (c >> 8) & 0xff;
                c3 = c & 0xff;
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
                c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
                WriteRGBPixel(rp, x0, py, c);
            }
        } else {
            y = (y0 - offy) % ni->h;
            for (px = x0; px < x1; px++) {
                x = px % ni->h;
                c = ni->data[x + y * ni->w];
                c0 = (c >> 24) & 0xff;
                c1 = (c >> 16) & 0xff;
                c2 = (c >> 8) & 0xff;
                c3 = c & 0xff;
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
                c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
                WriteRGBPixel(rp, px, y0, c);
            }
        }
    }
    else
    {
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
}

void DrawStatefulGadgetImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp)
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
            
        BltNewImageSubImageRastPortSimple(ni, subimagecol, subimagerow, rp, xp, yp);
    }
}
