/* 
    Copyright  1999, David Le Corfec.
    Copyright  2002-2018, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>

#include <stdio.h>

#include "muimaster_intern.h"
#include "datatypescache.h"
#include "mui.h"
#include "frame.h"
#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 custom frames
**************************************************************************/

void DrawPartToImage(struct NewImage *src, struct NewImage *dest, UWORD sx,
    UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy)
{
    UWORD x, y;

    ULONG *s, *d;



    for (y = 0; y < sh; y++)
    {
        s = &src->data[sx + ((sy + y) * src->w)];
        d = &dest->data[dx + ((dy + y) * dest->w)];

        for (x = 0; x < sw; x++)
        {
            *d++ = *s++;
        }
    }
}


static void DrawTileToImage(BOOL alpha, BOOL tc, struct RastPort *rp,
    struct NewImage *ni, struct NewImage *dest, UWORD _sx, UWORD _sy,
    UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh,
    UWORD posx, UWORD posy)
{

    ULONG dy, dx;
    LONG dh, height, dw, width;

    if (ni == NULL)
        return;

    if ((_sh == 0) || (_dh == 0) || (_sw == 0) || (_dw == 0))
        return;

    dh = _sh;
    dy = _dy;
    height = _dh;
    while (height > 0)
    {
        if ((height - dh) < 0)
            dh = height;
        height -= dh;

        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0)
        {
            if ((width - dw) < 0)
                dw = width;
            width -= dw;

            if (tc)
            {
                if (dest != NULL)
                {
                    DrawPartToImage(ni, dest, _sx, _sy, dw, dh, dx, dy);
                }
                else
                {
                    if (alpha)
                        WritePixelArrayAlpha(ni->data, _sx, _sy, ni->w * 4,
                            rp, dx + posx, dy + posy, dw, dh, 0xffffffff);
                    else
                        WritePixelArray(ni->data, _sx, _sy, ni->w * 4, rp,
                            dx + posx, dy + posy, dw, dh, RECTFMT_ARGB);
                }
            }
            else
            {
                if (ni->bitmap != NULL)
                {
                    if (alpha)
                    {
                        if (ni->mask)
                            BltMaskBitMapRastPort(ni->bitmap, _sx, _sy, rp,
                                dx + posx, dy + posy, dw, dh, 0xe0,
                                (PLANEPTR) ni->mask);
                        else
                            BltBitMapRastPort(ni->bitmap, _sx, _sy, rp,
                                dx + posx, dy + posy, dw, dh, 0xc0);
                    }
                    else
                    {
                        BltBitMapRastPort(ni->bitmap, _sx, _sy, rp,
                            dx + posx, dy + posy, dw, dh, 0xc0);
                    }
                }
            }
            dx += dw;
        }
        dy += dh;
    }
}

static void draw_tile_frame(struct RastPort *rport, BOOL tc, BOOL direct,
    BOOL alpha, struct dt_frame_image *fi, struct NewImage *src,
    struct NewImage *dest, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    int lw, rw, th, bh, right, bottom, gr, gb, lp, tp, rp, bp, mw, mh;


    right = left + width;
    bottom = top + height;

    gr = gl + gw;
    gb = gt + gh;

    /* calculate the left width of the frame */

    lw = left - gl;
    if (lw > fi->tile_left)
        lw = fi->tile_left;
    lw = fi->tile_left - lw;

    /* calculate the top height of the frame */

    th = top - gt;
    if (th > fi->tile_top)
        th = fi->tile_top;
    th = fi->tile_top - th;

    lp = fi->tile_left - lw;    // left position
    tp = fi->tile_top - th;

    if (right < (fi->tile_left + gl))
        lw -= ((fi->tile_left + gl) - right);
    if (bottom < (fi->tile_top + gt))
        th -= ((fi->tile_top + gt) - bottom);

    bp = src->h - fi->tile_bottom;
    bh = fi->tile_bottom;

    if (top > (gb - fi->tile_bottom))
    {
        bp += (top - (gb - fi->tile_bottom));
        bh -= (top - (gb - fi->tile_bottom));

        if (bottom < gb)
            bh -= (gb - bottom);
    }
    else
    {
        if (bottom < (gb - fi->tile_bottom))
            bh = 0;
        else
            bh -= (gb - bottom);
    }

    rp = src->w - fi->tile_right;
    rw = fi->tile_right;

    if (left > (gr - fi->tile_right))
    {
        rp += (left - (gr - fi->tile_right));
        rw -= (left - (gr - fi->tile_right));

        if (right < gr)
            rw -= (gr - right);
    }
    else
    {
        if (right < (gr - fi->tile_right))
            rw = 0;
        else
            rw -= (gr - right);
    }

    mw = width - lw - rw;
    mh = height - th - bh;

    struct NewImage *d;

    if (direct)
        d = NULL;
    else
        d = dest;

    DrawTileToImage(alpha, tc, rport, src, d, lp, tp, lw, th, 0, 0, lw, th,
        left, top);
    DrawTileToImage(alpha, tc, rport, src, d, lp, bp, lw, bh, 0,
        height - bh, lw, bh, left, top);
    DrawTileToImage(alpha, tc, rport, src, d, rp, tp, rw, th, width - rw, 0,
        rw, th, left, top);
    DrawTileToImage(alpha, tc, rport, src, d, rp, bp, rw, bh, width - rw,
        height - bh, rw, bh, left, top);

    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, tp,
        src->w - fi->tile_left - fi->tile_right, th, lw, 0, mw, th, left,
        top);
    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, bp,
        src->w - fi->tile_left - fi->tile_right, bh, lw, height - bh, mw,
        bh, left, top);

    DrawTileToImage(alpha, tc, rport, src, d, lp, fi->tile_top, lw,
        src->h - fi->tile_bottom - fi->tile_top, 0, th, lw, mh, left, top);

    DrawTileToImage(alpha, tc, rport, src, d, rp, fi->tile_top, rw,
        src->h - fi->tile_bottom - fi->tile_top, width - rw, th, rw, mh,
        left, top);
    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, fi->tile_top,
        src->w - fi->tile_left - fi->tile_right,
        src->h - fi->tile_bottom - fi->tile_top, lw, th, mw, mh, left, top);
}

struct FrameFillInfo
{
    struct Hook Hook;
    struct dt_frame_image *fi;
    struct NewImage *ni;
    WORD gl, gt, gw, gh, left, top, width, height, ox, oy;
};

struct BackFillMsg
{
    STACKED struct Layer *Layer;
    STACKED struct Rectangle Bounds;
    STACKED LONG OffsetX;
    STACKED LONG OffsetY;
};

AROS_UFH3S(void, WindowPatternBackFillFunc,
           AROS_UFHA(struct Hook *, Hook, A0),
           AROS_UFHA(struct RastPort *, RP, A2),
           AROS_UFHA(struct BackFillMsg *, BFM, A1))
{
    AROS_USERFUNC_INIT

    // get the data for our backfillhook
    struct FrameFillInfo *FFI = (struct FrameFillInfo *)Hook;

    ULONG depth = (ULONG) GetBitMapAttr(RP->BitMap, BMA_DEPTH);

    BOOL truecolor = TRUE;

    if (depth < 15)
        truecolor = FALSE;

    int left = BFM->Bounds.MinX;
    int top = BFM->Bounds.MinY;
    int width = BFM->Bounds.MaxX - left + 1;
    int height = BFM->Bounds.MaxY - top + 1;

    left -= FFI->ox;
    top -= FFI->oy;

    BOOL alpha = !FFI->fi->noalpha;
    BOOL direct = FALSE;

    if (!truecolor)
        direct = TRUE;

    if (!direct)
    {
        struct NewImage *dest = NewImageContainer(width, height);
        if (dest != NULL)
        {
            draw_tile_frame(NULL, truecolor, FALSE, alpha, FFI->fi, FFI->ni,
                dest, FFI->gl, FFI->gt, FFI->gw, FFI->gh, left, top, width,
                height);
            if (FFI->fi->noalpha)
                WritePixelArray(dest->data, 0, 0, dest->w * 4, RP, left, top,
                    width, height, RECTFMT_ARGB);
            else
                WritePixelArrayAlpha(dest->data, 0, 0, dest->w * 4, RP, left,
                    top, width, height, 0xffffffff);

            DisposeImageContainer(dest);
        }
        else
            direct = TRUE;
    }

    if (direct)
    {
        draw_tile_frame(RP, truecolor, FALSE, alpha, FFI->fi, FFI->ni, NULL,
            FFI->gl, FFI->gt, FFI->gw, FFI->gh, left, top, width, height);
    }

    AROS_USERFUNC_EXIT
}

void dt_do_frame_rects(struct RastPort *rp, struct dt_frame_image *fi,
    struct NewImage *ni, int gl, int gt, int gw, int gh, int left, int top,
    int width, int height)
{
    struct Rectangle rect;
    struct FrameFillInfo ffi;

    rect.MinX = left;
    rect.MinY = top;
    rect.MaxX = left + width - 1;
    rect.MaxY = top + height - 1;

    ffi.left = left;
    ffi.top = top;
    ffi.width = width;
    ffi.height = height;

    ffi.gl = gl;
    ffi.gt = gt;
    ffi.gw = gw;
    ffi.gh = gh;

    ffi.ox = 0;
    ffi.oy = 0;

    ffi.ni = ni;

    ffi.fi = fi;

    ffi.Hook.h_Entry = (HOOKFUNC) WindowPatternBackFillFunc;

    if (rp->Layer)
    {
        LockLayer(0, rp->Layer);
        ffi.ox = rp->Layer->bounds.MinX;
        ffi.oy = rp->Layer->bounds.MinY;
    }

    DoHookClipRects((struct Hook *)&ffi, rp, &rect);

    if (rp->Layer)
    {
        UnlockLayer(rp->Layer);
    }

}


static void frame_custom(struct dt_frame_image *fi, BOOL state,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    struct RastPort *rp = mri->mri_RastPort;
    struct NewImage *ni;

    if (fi == NULL)
        return;

    if (state)
        ni = fi->img_down;
    else
        ni = fi->img_up;

    if ((fi->tile_left + fi->tile_right) > gw)
        return;
    if ((fi->tile_top + fi->tile_bottom) > gh)
        return;


    if (ni != NULL)
    {
        dt_do_frame_rects(rp, fi, ni, gl, gt, gw, gh, left, top, width,
            height);
    }

}

static void frame_custom_up(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    frame_custom(fi, FALSE, mri, gl, gt, gw, gh, left, top, width, height);
}

static void frame_custom_down(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    frame_custom(fi, TRUE, mri, gl, gt, gw, gh, left, top, width, height);
}
/**************************************************************************
 no frame
**************************************************************************/
static void frame_none_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
}

/**************************************************************************
 1 : FST_RECT
**************************************************************************/
static void rect_draw(struct dt_frame_image *fi, struct MUI_RenderInfo *mri,
    int left, int top, int width, int height, MPen preset_color)
{
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[preset_color]);

    /* FIXME: usually RectFill() is faster */
    Move(rp, left, top);
    Draw(rp, left + width - 1, top);
    Draw(rp, left + width - 1, top + height - 1);
    Draw(rp, left, top + height - 1);
    Draw(rp, left, top);
}


/**************************************************************************
 simple white border
**************************************************************************/
static void frame_white_rect_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    rect_draw(fi, mri, left, top, width, height, MPEN_SHINE);
}

/**************************************************************************
 simple black border
**************************************************************************/
static void frame_black_rect_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
}

/**************************************************************************
 2 : FST_BEVEL

 Draw a bicolor rectangle
**************************************************************************/
static void button_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen ul_preset, MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[ul_preset]);
    Move(rp, left, top + height - 2);
    Draw(rp, left, top);
    Draw(rp, left + width - 2, top);

    SetAPen(rp, mri->mri_Pens[lr_preset]);
    Move(rp, left + width - 1, top);
    Draw(rp, left + width - 1, top + height - 1);
    Draw(rp, left, top + height - 1);

    SetAPen(rp, mri->mri_Pens[MPEN_BACKGROUND]);
    WritePixel(rp, left, top + height - 1);
    WritePixel(rp, left + width - 1, top);
}

/**************************************************************************
 classic button
**************************************************************************/
static void frame_bevelled_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    button_draw(fi, mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************
 classic pressed button
**************************************************************************/
static void frame_recessed_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    button_draw(fi, mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 3 : FST_THIN_BORDER
 Draw a thin relief border
**************************************************************************/
static void thinborder_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen ul_preset, MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    SetAPen(rp, mri->mri_Pens[ul_preset]);
    Move(rp, left, top + height - 1);
    Draw(rp, left, top);
    Draw(rp, left + width - 1, top);

    Move(rp, left + width - 2, top + 2);
    Draw(rp, left + width - 2, top + height - 2);
    Draw(rp, left + 2, top + height - 2);

    rect_draw(fi, mri, left + 1, top + 1, width - 1, height - 1, lr_preset);
}

/**************************************************************************
 draw border up
**************************************************************************/
static void frame_thin_border_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_SHINE,
        MPEN_SHADOW);
}

/**************************************************************************
 draw border down
**************************************************************************/
static void frame_thin_border_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_SHADOW,
        MPEN_SHINE);
}

/**************************************************************************
 4 : FST_THICK_BORDER
 Draw a thick relief border
**************************************************************************/
static void thickborder_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    BOOL bevelled)
{
    if (bevelled)
    {
        button_draw(fi, mri, left, top, width, height, MPEN_SHINE,
            MPEN_SHADOW);
        button_draw(fi, mri, left + 2, top + 2, width - 4, height - 4,
            MPEN_SHADOW, MPEN_SHINE);
    }
    else
    {
        button_draw(fi, mri, left, top, width, height, MPEN_SHADOW,
            MPEN_SHINE);
        button_draw(fi, mri, left + 2, top + 2, width - 4, height - 4,
            MPEN_SHINE, MPEN_SHADOW);
    }

    rect_draw(fi, mri, left + 1, top + 1, width - 2, height - 2,
        MPEN_BACKGROUND);
}

/**************************************************************************
 draw thick border up
**************************************************************************/
static void frame_thick_border_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thickborder_draw(fi, mri, left, top, width, height, TRUE);
}

/**************************************************************************
 draw thick border down
**************************************************************************/
static void frame_thick_border_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thickborder_draw(fi, mri, left, top, width, height, FALSE);
}

/**************************************************************************
 5 : FST_ROUND_BEVEL
**************************************************************************/
static void round_bevel_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen ul, MPen lr)
{
    SetAPen(mri->mri_RastPort, mri->mri_Pens[MPEN_BACKGROUND]);
    RectFill(mri->mri_RastPort, left, top, left + 3, top + height - 1);
    RectFill(mri->mri_RastPort,
        left + width - 4, top, left + width - 1, top + height - 1);
    rect_draw(fi, mri, left, top + 2, 2, height - 4, ul);
    rect_draw(fi, mri, left + 1, top + 1, 2, 1, ul);
    rect_draw(fi, mri, left + 1, top + height - 2, 2, 1, ul);
    rect_draw(fi, mri, left + 2, top + height - 1, 1, 1, ul);
    rect_draw(fi, mri, left + 2, top, width - 5, 1, ul);

    rect_draw(fi, mri, left + width - 2, top + 2, 2, height - 4, lr);
    rect_draw(fi, mri, left + width - 3, top + 1, 2, 1, lr);
    rect_draw(fi, mri, left + width - 3, top + height - 2, 2, 1, lr);
    rect_draw(fi, mri, left + 3, top + height - 1, width - 5, 1, lr);
    rect_draw(fi, mri, left + width - 3, top, 1, 1, lr);
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_bevel_draw(fi, mri, left, top, width, height,
        MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_bevel_draw(fi, mri, left, top, width, height,
        MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 6 : FST_WIN_BEVEL
**************************************************************************/
static void frame_border_button_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
    button_draw(fi, mri, left + 1, top + 1, width - 2, height - 2,
        MPEN_SHINE, MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_border_button_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    button_draw(fi, mri, left + 2, top + 2, width - 2, height - 2,
        MPEN_BACKGROUND, MPEN_SHADOW);
    button_draw(fi, mri, left + 1, top + 1, width - 1, height - 1,
        MPEN_HALFSHADOW, MPEN_SHADOW);
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
}

/**************************************************************************
 7 : FST_ROUND_THICK_BORDER
**************************************************************************/
static void round_thick_border_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    rect_draw(fi, mri, left, top, width - 2, height - 2, pen1);
    rect_draw(fi, mri, left + 2, top + 2, width - 2, height - 2, pen5);

    rect_draw(fi, mri, left, top, 2, 2, pen2);
    rect_draw(fi, mri, left, top + height - 3, 2, 2, pen2);
    rect_draw(fi, mri, left + width - 3, top, 2, 2, pen2);

    rect_draw(fi, mri, left + width - 2, top + height - 2, 2, 2, pen4);
    rect_draw(fi, mri, left + 1, top + height - 2, 2, 2, pen4);
    rect_draw(fi, mri, left + width - 2, top + 1, 2, 2, pen4);

    rect_draw(fi, mri, left + 1, top + 1, width - 2, height - 2, pen3);

    rect_draw(fi, mri, left + 2, top + 2, 2, 2, pen3);
    rect_draw(fi, mri, left + 2, top + height - 4, 2, 2, pen3);
    rect_draw(fi, mri, left + width - 4, top + height - 4, 2, 2, pen3);
    rect_draw(fi, mri, left + width - 4, top + 2, 2, 2, pen3);

    /* these points were not in the original frame.  -dlc */
    SetAPen(rp, mri->mri_Pens[pen5]);
    WritePixel(rp, left + 3, top + 3);

    SetAPen(rp, mri->mri_Pens[pen1]);
    WritePixel(rp, left + width - 4, top + height - 4);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thick_border_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_thick_border_draw(fi, mri, left, top, width, height,
        MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
        MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thick_border_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_thick_border_draw(fi, mri, left, top, width, height,
        MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
        MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 8 : FST_ROUND_THIN_BORDER
**************************************************************************/
static void round_thin_border_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    rect_draw(fi, mri, left, top, width - 1, height - 1, pen1);
    rect_draw(fi, mri, left + 1, top + 1, width - 1, height - 1, pen5);

    rect_draw(fi, mri, left, top, 2, 2, pen2);
    rect_draw(fi, mri, left + width - 4, top + height - 4, 2, 2, pen2);
    rect_draw(fi, mri, left + 2, top + 2, 2, 2, pen4);
    rect_draw(fi, mri, left + width - 2, top + height - 2, 2, 2, pen4);
    rect_draw(fi, mri, left + 1, top + 1, 2, 2, pen3);
    rect_draw(fi, mri, left + width - 3, top + height - 3, 2, 2, pen3);

    rect_draw(fi, mri, left + 1, top + height - 3, 1, 3, pen4);
    rect_draw(fi, mri, left + width - 3, top + 1, 3, 1, pen4);

    WritePixel(rp, left + 2, top + height - 3);
    WritePixel(rp, left + width - 3, top + 2);

    SetAPen(rp, mri->mri_Pens[pen2]);
    WritePixel(rp, left, top + height - 2);
    WritePixel(rp, left + 2, top + height - 2);
    WritePixel(rp, left + width - 2, top);
    WritePixel(rp, left + width - 2, top + 2);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_thin_border_draw(fi, mri, left, top, width, height,
        MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
        MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    round_thin_border_draw(fi, mri, left, top, width, height,
        MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
        MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 9 : FST_GRAY_BORDER
**************************************************************************/
static void frame_gray_border_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height,
        MPEN_HALFSHINE, MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_gray_border_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height,
        MPEN_HALFSHADOW, MPEN_HALFSHINE);
}

/**************************************************************************
 A : FST_SEMIROUND_BEVEL
**************************************************************************/
static void semiround_bevel_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int left, int top, int width, int height,
    MPen pen1, MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    button_draw(fi, mri, left, top, width, height, pen1, pen5);
    button_draw(fi, mri, left + 1, top + 1, width - 2, height - 2, pen2,
        pen4);

    SetAPen(rp, mri->mri_Pens[pen2]);
    WritePixel(rp, left, top);

    SetAPen(rp, mri->mri_Pens[pen1]);
    WritePixel(rp, left + 1, top + 1);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen5]);
    WritePixel(rp, left + width - 2, top + height - 2);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen4]);
    WritePixel(rp, left + width - 1, top + height - 1);

    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen3]);
    WritePixel(rp, left, top + height - 2);
    WritePixel(rp, left + 1, top + height - 1);
    WritePixel(rp, left + width - 2, top);
    WritePixel(rp, left + width - 1, top + 1);
}

/**************************************************************************

**************************************************************************/
static void frame_semiround_bevel_up_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    semiround_bevel_draw(fi, mri, left, top, width, height,
        MPEN_SHINE, MPEN_HALFSHINE, MPEN_BACKGROUND,
        MPEN_HALFSHADOW, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_semiround_bevel_down_draw(struct dt_frame_image *fi,
    struct MUI_RenderInfo *mri, int gl, int gt, int gw, int gh, int left,
    int top, int width, int height)
{
    semiround_bevel_draw(fi, mri, left, top, width, height,
        MPEN_SHADOW, MPEN_HALFSHADOW, MPEN_BACKGROUND,
        MPEN_HALFSHINE, MPEN_SHINE);
}

/**************************************************************************
 hold builtin frames.
**************************************************************************/
static const struct ZuneFrameGfx __builtinFrameGfx[] = {
    /* type 0 : FST_NONE */
    {frame_none_draw, 0, 0, 0, 0, 0},
    {frame_none_draw, 0, 0, 0, 0, 0},

    /* monochrome border */
    /* 1 : FST_RECT */
    {frame_white_rect_draw, 0, 1, 1, 1, 1},
    {frame_black_rect_draw, 0, 1, 1, 1, 1},

    /* clean 3D look */
    /* 2 : FST_BEVEL */
    {frame_bevelled_draw, 0, 1, 1, 1, 1},
    {frame_recessed_draw, 0, 1, 1, 1, 1},

    /* thin relief border */
    /* 3 : FST_THIN_BORDER */
    {frame_thin_border_up_draw, 0, 2, 2, 2, 2},
    {frame_thin_border_down_draw, 0, 2, 2, 2, 2},

    /* thick relief border */
    /* 4 : FST_THICK_BORDER */
    {frame_thick_border_up_draw, 0, 3, 3, 3, 3},
    {frame_thick_border_down_draw, 0, 3, 3, 3, 3},

    /* rounded bevel */
    /* 5 : FST_ROUND_BEVEL */
    {frame_round_bevel_up_draw, 0, 4, 4, 1, 1},
    {frame_round_bevel_down_draw, 0, 4, 4, 1, 1},

    /* zin31/xen look */
    /* 6 : FST_WIN_BEVEL */
    {frame_border_button_up_draw, 0, 2, 2, 2, 2},
    {frame_border_button_down_draw, 0, 3, 1, 3, 1},

    /* rounded thick border */
    /* 7 : FST_ROUND_THICK_BORDER */
    {frame_round_thick_border_up_draw, 0, 4, 4, 4, 4},
    {frame_round_thick_border_down_draw, 0, 4, 4, 4, 4},

    /* rounded thin border */
    /* 8 : FST_ROUND_THIN_BORDER */
    {frame_round_thin_border_up_draw, 0, 4, 4, 4, 4},
    {frame_round_thin_border_down_draw, 0, 4, 4, 4, 4},

    /* strange gray border */
    /* 9 : FST_GRAY_BORDER */
    {frame_gray_border_up_draw, 0, 2, 2, 2, 2},
    {frame_gray_border_down_draw, 0, 2, 2, 2, 2},

    /* semi rounded bevel */
    /* A : FST_SEMIROUND_BEVEL */
    {frame_semiround_bevel_up_draw, 0, 2, 2, 2, 2},
    {frame_semiround_bevel_down_draw, 0, 2, 2, 2, 2},

    /* custom frames */

    {frame_custom_up, 1, 0, 0, 0, 0},
    {frame_custom_down, 1, 0, 0, 0, 0},

    {frame_custom_up, 2, 0, 0, 0, 0},
    {frame_custom_down, 2, 0, 0, 0, 0},

    {frame_custom_up, 3, 0, 0, 0, 0},
    {frame_custom_down, 3, 0, 0, 0, 0},

    {frame_custom_up, 4, 0, 0, 0, 0},
    {frame_custom_down, 4, 0, 0, 0, 0},

    {frame_custom_up, 5, 0, 0, 0, 0},
    {frame_custom_down, 5, 0, 0, 0, 0},

    {frame_custom_up, 6, 0, 0, 0, 0},
    {frame_custom_down, 6, 0, 0, 0, 0},

    {frame_custom_up, 7, 0, 0, 0, 0},
    {frame_custom_down, 7, 0, 0, 0, 0},

    {frame_custom_up, 8, 0, 0, 0, 0},
    {frame_custom_down, 8, 0, 0, 0, 0},

    {frame_custom_up, 9, 0, 0, 0, 0},
    {frame_custom_down, 9, 0, 0, 0, 0},

    {frame_custom_up, 10, 0, 0, 0, 0},
    {frame_custom_down, 10, 0, 0, 0, 0},

    {frame_custom_up, 11, 0, 0, 0, 0},
    {frame_custom_down, 11, 0, 0, 0, 0},

    {frame_custom_up, 12, 0, 0, 0, 0},
    {frame_custom_down, 12, 0, 0, 0, 0},

    {frame_custom_up, 13, 0, 0, 0, 0},
    {frame_custom_down, 13, 0, 0, 0, 0},

    {frame_custom_up, 14, 0, 0, 0, 0},
    {frame_custom_down, 14, 0, 0, 0, 0},

    {frame_custom_up, 15, 0, 0, 0, 0},
    {frame_custom_down, 15, 0, 0, 0, 0},

    {frame_custom_up, 16, 0, 0, 0, 0},
    {frame_custom_down, 16, 0, 0, 0, 0},

};


/**************************************************************************

**************************************************************************/
const struct ZuneFrameGfx *zune_zframe_get(Object *obj,
    const struct MUI_FrameSpec_intern *frameSpec)
{
    struct dt_frame_image *fi = NULL;

    if (frameSpec->type >= FST_CUSTOM1)
    {
        struct MUI_RenderInfo *mri = muiRenderInfo(obj);
        if (!(fi = mri->mri_FrameImage[frameSpec->type - FST_CUSTOM1]))
            return &__builtinFrameGfx[2 * FST_RECT];
    }

    if (frameSpec->type >= FST_COUNT)
        return &__builtinFrameGfx[2 * FST_RECT];

    return &__builtinFrameGfx[2 * frameSpec->type + frameSpec->state];

#if 0
    frame->customframe = NULL;

    if ((fi != NULL) && (frame != NULL))
    {
        frame->customframe = fi;
        frame->ileft = fi->inner_left;
        frame->itop = fi->inner_top;
        frame->iright = fi->inner_right;
        frame->ibottom = fi->inner_bottom;
        frame->noalpha = fi->noalpha;

    }

    return frame;
#endif
}

const struct ZuneFrameGfx *zune_zframe_get_with_state(Object *obj,
    const struct MUI_FrameSpec_intern *frameSpec, UWORD state)
{
    struct dt_frame_image *fi = NULL;

    if (frameSpec->type >= FST_CUSTOM1)
    {
        struct MUI_RenderInfo *mri = muiRenderInfo(obj);
        if (!(fi = mri->mri_FrameImage[frameSpec->type - FST_CUSTOM1]))
            return &__builtinFrameGfx[2 * FST_RECT];
    }

    if (frameSpec->type >= FST_COUNT)
        return &__builtinFrameGfx[2 * FST_RECT];

    return &__builtinFrameGfx[2 * frameSpec->type + state];
}

/*------------------------------------------------------------------------*/

BOOL zune_frame_intern_to_spec(const struct MUI_FrameSpec_intern *intern,
    STRPTR spec)
{
     if (!intern || !spec)
        return FALSE;

    char tmpSpec[3];
    char sp;

    if (intern->type < 10)
        sp = ((char)intern->type) + '0';
    else
        sp = ((char)intern->type - 10) + 'a';

    /* Must cast to LONG because on AmigaOS SNPrintf() is used which is like
     * RawDoFmt() 16 bit */
    snprintf(&spec[0], 2, "%c", sp);
    snprintf(tmpSpec, 3, "%lx", (long)intern->state);
    spec[1] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (long)intern->innerLeft);
    spec[2] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (long)intern->innerRight);
    spec[3] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (long)intern->innerTop);
    spec[4] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (long)intern->innerBottom);
    spec[5] = tmpSpec[0];
    spec[6] = '\0';
    return TRUE;
}

/*------------------------------------------------------------------------*/

static int xhexasciichar_to_int(char x)
{
    if (x >= '0' && x <= '9')
        return x - '0';
    if (x >= 'a' && x <= 'z')
        return x - 'a' + 10;
    if (x >= 'A' && x <= 'Z')
        return x - 'A' + 10;
    return -1;
}

BOOL zune_frame_spec_to_intern(CONST_STRPTR spec,
    struct MUI_FrameSpec_intern *intern)
{
    int val;

    if (!intern || !spec)
        return FALSE;

    val = xhexasciichar_to_int(spec[0]);
    if (val == -1)
        return FALSE;
    intern->type = val;

    val = xhexasciichar_to_int(spec[1]);
    if (val == -1)
        return FALSE;
    intern->state = val;

    val = xhexasciichar_to_int(spec[2]);
    if (val == -1)
        return FALSE;
    intern->innerLeft = val;

    val = xhexasciichar_to_int(spec[3]);
    if (val == -1)
        return FALSE;
    intern->innerRight = val;

    val = xhexasciichar_to_int(spec[4]);
    if (val == -1)
        return FALSE;
    intern->innerTop = val;

    val = xhexasciichar_to_int(spec[5]);
    if (val == -1)
        return FALSE;
    intern->innerBottom = val;
    return TRUE;
}
