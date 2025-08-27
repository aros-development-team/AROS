/*
    Copyright  2002-2020, The AROS Development Team.
    Copyright  1999, David Le Corfec.
    All rights reserved.

*/

#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <stdio.h>
#include <string.h>

#include "datatypescache.h"
#include "frame.h"
#include "mui.h"
#include "muimaster_intern.h"

#define DEBUG 0
#include <aros/debug.h>

extern struct Library *MUIMasterBase;

/**************************************************************************
 custom frames
**************************************************************************/

void DrawPartToImage(struct NewImage *src, struct NewImage *dest, UWORD sx,
                     UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy)
{
    UWORD x, y;

    ULONG *s, *d;

    for (y = 0; y < sh; y++) {
        s = &src->data[sx + ((sy + y) * src->w)];
        d = &dest->data[dx + ((dy + y) * dest->w)];

        for (x = 0; x < sw; x++) {
            *d++ = *s++;
        }
    }
}

static void DrawTileToImage(BOOL alpha, BOOL tc, struct RastPort *rp,
                            struct NewImage *ni, struct NewImage *dest,
                            UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh,
                            UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh,
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
    while (height > 0) {
        if ((height - dh) < 0)
            dh = height;
        height -= dh;

        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0) {
            if ((width - dw) < 0)
                dw = width;
            width -= dw;

            if (tc) {
                if (dest != NULL) {
                    DrawPartToImage(ni, dest, _sx, _sy, dw, dh, dx, dy);
                } else {
                    if (alpha)
                        WritePixelArrayAlpha(ni->data, _sx, _sy, ni->w * 4, rp, dx + posx,
                                             dy + posy, dw, dh, 0xffffffff);
                    else
                        WritePixelArray(ni->data, _sx, _sy, ni->w * 4, rp, dx + posx,
                                        dy + posy, dw, dh, RECTFMT_ARGB);
                }
            } else {
                if (ni->bitmap != NULL) {
                    if (alpha) {
                        if (ni->mask)
                            BltMaskBitMapRastPort(ni->bitmap, _sx, _sy, rp, dx + posx,
                                                  dy + posy, dw, dh, 0xe0,
                                                  (PLANEPTR)ni->mask);
                        else
                            BltBitMapRastPort(ni->bitmap, _sx, _sy, rp, dx + posx, dy + posy,
                                              dw, dh, 0xc0);
                    } else {
                        BltBitMapRastPort(ni->bitmap, _sx, _sy, rp, dx + posx, dy + posy,
                                          dw, dh, 0xc0);
                    }
                }
            }
            dx += dw;
        }
        dy += dh;
    }
}

static void draw_tile_frame(struct RastPort *rport, BOOL tc, BOOL direct,
                            BOOL alpha, struct dt_frame_image *fi,
                            struct NewImage *src, struct NewImage *dest, int gl,
                            int gt, int gw, int gh, int left, int top,
                            int width, int height)
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

    lp = fi->tile_left - lw; // left position
    tp = fi->tile_top - th;

    if (right < (fi->tile_left + gl))
        lw -= ((fi->tile_left + gl) - right);
    if (bottom < (fi->tile_top + gt))
        th -= ((fi->tile_top + gt) - bottom);

    bp = src->h - fi->tile_bottom;
    bh = fi->tile_bottom;

    if (top > (gb - fi->tile_bottom)) {
        bp += (top - (gb - fi->tile_bottom));
        bh -= (top - (gb - fi->tile_bottom));

        if (bottom < gb)
            bh -= (gb - bottom);
    } else {
        if (bottom < (gb - fi->tile_bottom))
            bh = 0;
        else
            bh -= (gb - bottom);
    }

    rp = src->w - fi->tile_right;
    rw = fi->tile_right;

    if (left > (gr - fi->tile_right)) {
        rp += (left - (gr - fi->tile_right));
        rw -= (left - (gr - fi->tile_right));

        if (right < gr)
            rw -= (gr - right);
    } else {
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

    DrawTileToImage(alpha, tc, rport, src, d, lp, tp, lw, th, 0, 0, lw, th, left,
                    top);
    DrawTileToImage(alpha, tc, rport, src, d, lp, bp, lw, bh, 0, height - bh, lw,
                    bh, left, top);
    DrawTileToImage(alpha, tc, rport, src, d, rp, tp, rw, th, width - rw, 0, rw,
                    th, left, top);
    DrawTileToImage(alpha, tc, rport, src, d, rp, bp, rw, bh, width - rw,
                    height - bh, rw, bh, left, top);

    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, tp,
                    src->w - fi->tile_left - fi->tile_right, th, lw, 0, mw, th,
                    left, top);
    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, bp,
                    src->w - fi->tile_left - fi->tile_right, bh, lw, height - bh,
                    mw, bh, left, top);

    DrawTileToImage(alpha, tc, rport, src, d, lp, fi->tile_top, lw,
                    src->h - fi->tile_bottom - fi->tile_top, 0, th, lw, mh, left,
                    top);

    DrawTileToImage(alpha, tc, rport, src, d, rp, fi->tile_top, rw,
                    src->h - fi->tile_bottom - fi->tile_top, width - rw, th, rw,
                    mh, left, top);
    DrawTileToImage(alpha, tc, rport, src, d, fi->tile_left, fi->tile_top,
                    src->w - fi->tile_left - fi->tile_right,
                    src->h - fi->tile_bottom - fi->tile_top, lw, th, mw, mh, left,
                    top);
}

struct FrameFillInfo {
    struct Hook Hook;
    struct dt_frame_image *fi;
    struct NewImage *ni;
    WORD gl, gt, gw, gh, left, top, width, height, ox, oy;
};

struct BackFillMsg {
    STACKED struct Layer *Layer;
    STACKED struct Rectangle Bounds;
    STACKED LONG OffsetX;
    STACKED LONG OffsetY;
};

AROS_UFH3S(void, WindowPatternBackFillFunc, AROS_UFHA(struct Hook *, Hook, A0),
           AROS_UFHA(struct RastPort *, RP, A2),
           AROS_UFHA(struct BackFillMsg *, BFM, A1))
{
    AROS_USERFUNC_INIT

    // get the data for our backfillhook
    struct FrameFillInfo *FFI = (struct FrameFillInfo *)Hook;

    ULONG depth = (ULONG)GetBitMapAttr(RP->BitMap, BMA_DEPTH);

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

    if (!direct) {
        struct NewImage *dest = NewImageContainer(width, height);
        if (dest != NULL) {
            draw_tile_frame(NULL, truecolor, FALSE, alpha, FFI->fi, FFI->ni, dest,
                            FFI->gl, FFI->gt, FFI->gw, FFI->gh, left, top, width,
                            height);
            if (FFI->fi->noalpha)
                WritePixelArray(dest->data, 0, 0, dest->w * 4, RP, left, top, width,
                                height, RECTFMT_ARGB);
            else
                WritePixelArrayAlpha(dest->data, 0, 0, dest->w * 4, RP, left, top,
                                     width, height, 0xffffffff);

            DisposeImageContainer(dest);
        } else
            direct = TRUE;
    }

    if (direct) {
        draw_tile_frame(RP, truecolor, FALSE, alpha, FFI->fi, FFI->ni, NULL,
                        FFI->gl, FFI->gt, FFI->gw, FFI->gh, left, top, width,
                        height);
    }

    AROS_USERFUNC_EXIT
}

void dt_do_frame_rects(struct RastPort *rp, struct dt_frame_image *fi,
                       struct NewImage *ni, int gl, int gt, int gw, int gh,
                       int left, int top, int width, int height)
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

    ffi.Hook.h_Entry = (HOOKFUNC)WindowPatternBackFillFunc;

    if (rp->Layer) {
        LockLayer(0, rp->Layer);
        ffi.ox = rp->Layer->bounds.MinX;
        ffi.oy = rp->Layer->bounds.MinY;
    }

    DoHookClipRects((struct Hook *)&ffi, rp, &rect);

    if (rp->Layer) {
        UnlockLayer(rp->Layer);
    }
}

static void frame_custom(struct dt_frame_image *fi, BOOL state,
                         struct MUI_RenderInfo *mri, int gl, int gt, int gw,
                         int gh, int left, int top, int width, int height)
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

    if (ni != NULL) {
        dt_do_frame_rects(rp, fi, ni, gl, gt, gw, gh, left, top, width, height);
    }
}

static void frame_custom_up(struct dt_frame_image *fi,
                            struct MUI_RenderInfo *mri, int gl, int gt, int gw,
                            int gh, int left, int top, int width, int height)
{
    frame_custom(fi, FALSE, mri, gl, gt, gw, gh, left, top, width, height);
}

static void frame_custom_down(struct dt_frame_image *fi,
                              struct MUI_RenderInfo *mri, int gl, int gt,
                              int gw, int gh, int left, int top, int width,
                              int height)
{
    frame_custom(fi, TRUE, mri, gl, gt, gw, gh, left, top, width, height);
}
/**************************************************************************
 no frame
**************************************************************************/
/**************************************************************************
 Drawing primitives - optimized for performance
**************************************************************************/

/** Horizontal line drawing */
static inline void draw_horizontal_line(struct RastPort *rp, int x1, int x2,
                                        int y, ULONG pen)
{
    if (x1 > x2)
        return; /* Safety check */
    SetAPen(rp, pen);
    RectFill(rp, x1, y, x2, y);
}

/** Vertical line drawing */
static inline void draw_vertical_line(struct RastPort *rp, int x, int y1,
                                      int y2, ULONG pen)
{
    if (y1 > y2)
        return; /* Safety check */
    SetAPen(rp, pen);
    RectFill(rp, x, y1, x, y2);
}

/**
 * Draw 3D edge effect
 * @param raised TRUE for raised effect, FALSE for recessed
 */
static void draw_3d_edge(struct MUI_RenderInfo *mri, int left, int top,
                         int width, int height, ULONG light_pen, ULONG dark_pen,
                         BOOL raised)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (width <= 0 || height <= 0)
        return; /* Safety check */

    if (raised) {
        /* Draw light edges (top and left) */
        draw_horizontal_line(rp, left, left + width - 2, top, light_pen);
        draw_vertical_line(rp, left, top, top + height - 2, light_pen);

        /* Draw dark edges (bottom and right) */
        draw_horizontal_line(rp, left + 1, left + width - 1, top + height - 1,
                             dark_pen);
        draw_vertical_line(rp, left + width - 1, top, top + height - 1, dark_pen);
    } else {
        /* Draw dark edges (top and left) */
        draw_horizontal_line(rp, left, left + width - 2, top, dark_pen);
        draw_vertical_line(rp, left, top, top + height - 2, dark_pen);

        /* Draw light edges (bottom and right) */
        draw_horizontal_line(rp, left + 1, left + width - 1, top + height - 1,
                             light_pen);
        draw_vertical_line(rp, left + width - 1, top, top + height - 1, light_pen);
    }
}

/**************************************************************************
 0 : FST_NONE
**************************************************************************/
static void frame_none_draw(struct dt_frame_image *fi,
                            struct MUI_RenderInfo *mri, int gl, int gt, int gw,
                            int gh, int left, int top, int width, int height)
{
    /* No frame - nothing to draw */
}

/**************************************************************************
 1 : FST_RECT - Rectangle border drawing
**************************************************************************/
static void rect_draw(struct dt_frame_image *fi, struct MUI_RenderInfo *mri,
                      int left, int top, int width, int height,
                      MPen preset_color)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (width <= 0 || height <= 0)
        return; /* Safety check */

    SetAPen(rp, mri->mri_Pens[preset_color]);

    if (width == 1 || height == 1) {
        /* For thin lines, fill the entire area */
        RectFill(rp, left, top, left + width - 1, top + height - 1);
    } else {
        /* For borders, draw individual edges efficiently */
        RectFill(rp, left, top, left + width - 1, top); /* Top edge */
        RectFill(rp, left, top + height - 1, left + width - 1,
                 top + height - 1);                          /* Bottom edge */
        RectFill(rp, left, top + 1, left, top + height - 2); /* Left edge */
        RectFill(rp, left + width - 1, top + 1, left + width - 1,
                 top + height - 2); /* Right edge */
    }
}

/**************************************************************************
 simple white border
**************************************************************************/
static void frame_white_rect_draw(struct dt_frame_image *fi,
                                  struct MUI_RenderInfo *mri, int gl, int gt,
                                  int gw, int gh, int left, int top, int width,
                                  int height)
{
    rect_draw(fi, mri, left, top, width, height, MPEN_SHINE);
}

/**************************************************************************
 simple black border
**************************************************************************/
static void frame_black_rect_draw(struct dt_frame_image *fi,
                                  struct MUI_RenderInfo *mri, int gl, int gt,
                                  int gw, int gh, int left, int top, int width,
                                  int height)
{
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
}

/**************************************************************************
 2 : FST_BEVEL

 Draw a bicolor rectangle
**************************************************************************/
/**
 * Draw a basic 3D button effect
 * @param ul_preset Upper-left edge pen (light for raised, dark for recessed)
 * @param lr_preset Lower-right edge pen (dark for raised, light for recessed)
 */
static void button_draw(struct dt_frame_image *fi, struct MUI_RenderInfo *mri,
                        int left, int top, int width, int height,
                        MPen ul_preset, MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (!rp || width <= 2 || height <= 2)
        return; /* Safety check - need minimum size and valid rastport */

    /* Draw upper-left edges */
    SetAPen(rp, mri->mri_Pens[ul_preset]);
    draw_vertical_line(rp, left, top, top + height - 2, mri->mri_Pens[ul_preset]);
    draw_horizontal_line(rp, left, left + width - 2, top,
                         mri->mri_Pens[ul_preset]);

    /* Draw lower-right edges */
    SetAPen(rp, mri->mri_Pens[lr_preset]);
    draw_vertical_line(rp, left + width - 1, top, top + height - 1,
                       mri->mri_Pens[lr_preset]);
    draw_horizontal_line(rp, left, left + width - 1, top + height - 1,
                         mri->mri_Pens[lr_preset]);

    /* Fix corner pixels with background color */
    SetAPen(rp, mri->mri_Pens[MPEN_BACKGROUND]);
    WritePixel(rp, left, top + height - 1);
    WritePixel(rp, left + width - 1, top);
}

/**************************************************************************
 classic button
**************************************************************************/
static void frame_bevelled_draw(struct dt_frame_image *fi,
                                struct MUI_RenderInfo *mri, int gl, int gt,
                                int gw, int gh, int left, int top, int width,
                                int height)
{
    button_draw(fi, mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************
 classic pressed button
**************************************************************************/
static void frame_recessed_draw(struct dt_frame_image *fi,
                                struct MUI_RenderInfo *mri, int gl, int gt,
                                int gw, int gh, int left, int top, int width,
                                int height)
{
    button_draw(fi, mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 3 : FST_THIN_BORDER
 Draw a thin relief border
**************************************************************************/
/**
 * Draw a thin 3D border - optimized version
 */
static void thinborder_draw(struct dt_frame_image *fi,
                            struct MUI_RenderInfo *mri, int left, int top,
                            int width, int height, MPen ul_preset,
                            MPen lr_preset)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (width <= 3 || height <= 3)
        return; /* Safety check */

    /* Draw outer upper-left edges */
    SetAPen(rp, mri->mri_Pens[ul_preset]);
    draw_vertical_line(rp, left, top, top + height - 1, mri->mri_Pens[ul_preset]);
    draw_horizontal_line(rp, left, left + width - 1, top,
                         mri->mri_Pens[ul_preset]);

    /* Draw inner upper-left edges */
    draw_vertical_line(rp, left + width - 2, top + 2, top + height - 2,
                       mri->mri_Pens[ul_preset]);
    draw_horizontal_line(rp, left + 2, left + width - 2, top + height - 2,
                         mri->mri_Pens[ul_preset]);

    /* Draw the inner border with lower-right pen */
    rect_draw(fi, mri, left + 1, top + 1, width - 1, height - 1, lr_preset);
}

/**************************************************************************
 draw border up
**************************************************************************/
static void frame_thin_border_up_draw(struct dt_frame_image *fi,
                                      struct MUI_RenderInfo *mri, int gl,
                                      int gt, int gw, int gh, int left, int top,
                                      int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************
 draw border down
**************************************************************************/
static void frame_thin_border_down_draw(struct dt_frame_image *fi,
                                        struct MUI_RenderInfo *mri, int gl,
                                        int gt, int gw, int gh, int left,
                                        int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 4 : FST_THICK_BORDER
 Draw a thick relief border
**************************************************************************/
/**
 * Draw a thick 3D border with double bevel effect
 * @param bevelled TRUE for raised appearance, FALSE for recessed
 */
static void thickborder_draw(struct dt_frame_image *fi,
                             struct MUI_RenderInfo *mri, int left, int top,
                             int width, int height, BOOL bevelled)
{
    if (width <= 4 || height <= 4)
        return; /* Safety check - need minimum size */

    if (bevelled) {
        /* Raised: outer light, inner dark */
        button_draw(fi, mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
        button_draw(fi, mri, left + 2, top + 2, width - 4, height - 4, MPEN_SHADOW,
                    MPEN_SHINE);
    } else {
        /* Recessed: outer dark, inner light */
        button_draw(fi, mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
        button_draw(fi, mri, left + 2, top + 2, width - 4, height - 4, MPEN_SHINE,
                    MPEN_SHADOW);
    }

    /* Fill the middle border area with background */
    rect_draw(fi, mri, left + 1, top + 1, width - 2, height - 2, MPEN_BACKGROUND);
}

/**************************************************************************
 draw thick border up
**************************************************************************/
static void frame_thick_border_up_draw(struct dt_frame_image *fi,
                                       struct MUI_RenderInfo *mri, int gl,
                                       int gt, int gw, int gh, int left,
                                       int top, int width, int height)
{
    thickborder_draw(fi, mri, left, top, width, height, TRUE);
}

/**************************************************************************
 draw thick border down
**************************************************************************/
static void frame_thick_border_down_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    thickborder_draw(fi, mri, left, top, width, height, FALSE);
}

/**************************************************************************
 5 : FST_ROUND_BEVEL
**************************************************************************/
/**
 * Draw a rounded bevel frame
 * @param ul Upper-left pen (light for raised, dark for recessed)
 * @param lr Lower-right pen (dark for raised, light for recessed)
 */
static void round_bevel_draw(struct dt_frame_image *fi,
                             struct MUI_RenderInfo *mri, int left, int top,
                             int width, int height, MPen ul, MPen lr)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (width <= 8 || height <= 4)
        return; /* Safety check - need minimum size for rounded effect */

    /* Clear corner areas with background - grouped operations */
    SetAPen(rp, mri->mri_Pens[MPEN_BACKGROUND]);
    RectFill(rp, left, top, left + 3, top + height - 1);
    RectFill(rp, left + width - 4, top, left + width - 1, top + height - 1);

    /* Draw upper-left edges and corners - grouped by pen */
    SetAPen(rp, mri->mri_Pens[ul]);
    RectFill(rp, left, top + 2, left + 1, top + height - 3); /* Left edge */
    RectFill(rp, left + 1, top + 1, left + 2, top + 1);      /* Top-left corner */
    RectFill(rp, left + 1, top + height - 2, left + 2,
             top + height - 2); /* Bottom-left corner */
    RectFill(rp, left + 2, top + height - 1, left + 2,
             top + height - 1);                         /* Bottom-left pixel */
    RectFill(rp, left + 2, top, left + width - 6, top); /* Top edge */

    /* Draw lower-right edges and corners - grouped by pen */
    SetAPen(rp, mri->mri_Pens[lr]);
    RectFill(rp, left + width - 2, top + 2, left + width - 1,
             top + height - 3); /* Right edge */
    RectFill(rp, left + width - 3, top + 1, left + width - 2,
             top + 1); /* Top-right corner */
    RectFill(rp, left + width - 3, top + height - 2, left + width - 2,
             top + height - 2); /* Bottom-right corner */
    RectFill(rp, left + 3, top + height - 1, left + width - 6,
             top + height - 1); /* Bottom edge */
    RectFill(rp, left + width - 3, top, left + width - 3,
             top); /* Top-right pixel */
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_up_draw(struct dt_frame_image *fi,
                                      struct MUI_RenderInfo *mri, int gl,
                                      int gt, int gw, int gh, int left, int top,
                                      int width, int height)
{
    round_bevel_draw(fi, mri, left, top, width, height, MPEN_SHINE, MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_bevel_down_draw(struct dt_frame_image *fi,
                                        struct MUI_RenderInfo *mri, int gl,
                                        int gt, int gw, int gh, int left,
                                        int top, int width, int height)
{
    round_bevel_draw(fi, mri, left, top, width, height, MPEN_SHADOW, MPEN_SHINE);
}

/**************************************************************************
 6 : FST_WIN_BEVEL
**************************************************************************/
/**
 * Windows-style button frame (raised) - FST_WIN_BEVEL
 */
static void frame_border_button_up_draw(struct dt_frame_image *fi,
                                        struct MUI_RenderInfo *mri, int gl,
                                        int gt, int gw, int gh, int left,
                                        int top, int width, int height)
{
    if (width <= 2 || height <= 2)
        return; /* Safety check */

    /* Outer dark border */
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
    /* Inner raised bevel */
    button_draw(fi, mri, left + 1, top + 1, width - 2, height - 2, MPEN_SHINE,
                MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
/**
 * Windows-style button frame (pressed) - FST_WIN_BEVEL
 */
static void frame_border_button_down_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    if (width <= 4 || height <= 4)
        return; /* Safety check */

    /* Draw from outside to inside for better visual layering */
    rect_draw(fi, mri, left, top, width, height, MPEN_SHADOW);
    button_draw(fi, mri, left + 1, top + 1, width - 1, height - 1,
                MPEN_HALFSHADOW, MPEN_SHADOW);
    button_draw(fi, mri, left + 2, top + 2, width - 2, height - 2,
                MPEN_BACKGROUND, MPEN_SHADOW);
}

/**************************************************************************
 7 : FST_ROUND_THICK_BORDER
**************************************************************************/
static void round_thick_border_draw(struct dt_frame_image *fi,
                                    struct MUI_RenderInfo *mri, int left,
                                    int top, int width, int height, MPen pen1,
                                    MPen pen2, MPen pen3, MPen pen4,
                                    MPen pen5)
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
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    round_thick_border_draw(fi, mri, left, top, width, height, MPEN_SHINE,
                            MPEN_HALFSHINE, MPEN_BACKGROUND, MPEN_HALFSHADOW,
                            MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thick_border_down_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri,
        int gl, int gt, int gw, int gh,
        int left, int top, int width,
        int height)
{
    round_thick_border_draw(fi, mri, left, top, width, height, MPEN_SHADOW,
                            MPEN_HALFSHADOW, MPEN_BACKGROUND, MPEN_HALFSHINE,
                            MPEN_SHINE);
}

/**************************************************************************
 8 : FST_ROUND_THIN_BORDER
**************************************************************************/
/**
 * Draw a rounded thin border - complex multi-pen effect
 * Uses 5 pens to create gradient-like rounded appearance
 */
static void round_thin_border_draw(struct dt_frame_image *fi,
                                   struct MUI_RenderInfo *mri, int left,
                                   int top, int width, int height, MPen pen1,
                                   MPen pen2, MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    if (width <= 6 || height <= 6)
        return; /* Safety check */

    /* Draw main border lines */
    rect_draw(fi, mri, left, top, width - 1, height - 1, pen1);
    rect_draw(fi, mri, left + 1, top + 1, width - 1, height - 1, pen5);

    /* Draw outer corners with pen2 */
    SetAPen(rp, mri->mri_Pens[pen2]);
    RectFill(rp, left, top, left + 1, top + 1); /* Top-left outer */
    RectFill(rp, left + width - 4, top + height - 4, left + width - 3,
             top + height - 3); /* Bottom-right outer */
    WritePixel(rp, left, top + height - 2);
    WritePixel(rp, left + 2, top + height - 2);
    WritePixel(rp, left + width - 2, top);
    WritePixel(rp, left + width - 2, top + 2);

    /* Draw middle corners with pen3 */
    SetAPen(rp, mri->mri_Pens[pen3]);
    RectFill(rp, left + 1, top + 1, left + 2, top + 2); /* Top-left middle */
    RectFill(rp, left + width - 3, top + height - 3, left + width - 2,
             top + height - 2); /* Bottom-right middle */

    /* Draw inner corners and edges with pen4 */
    SetAPen(rp, mri->mri_Pens[pen4]);
    RectFill(rp, left + 2, top + 2, left + 3, top + 3); /* Top-left inner */
    RectFill(rp, left + width - 2, top + height - 2, left + width - 1,
             top + height - 1); /* Bottom-right inner */
    RectFill(rp, left + 1, top + height - 3, left + 1,
             top + height - 1); /* Left edge */
    RectFill(rp, left + width - 3, top + 1, left + width - 1,
             top + 1); /* Top edge */
    WritePixel(rp, left + 2, top + height - 3);
    WritePixel(rp, left + width - 3, top + 2);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_up_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    round_thin_border_draw(fi, mri, left, top, width, height, MPEN_SHINE,
                           MPEN_HALFSHINE, MPEN_BACKGROUND, MPEN_HALFSHADOW,
                           MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_round_thin_border_down_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri,
        int gl, int gt, int gw, int gh,
        int left, int top, int width,
        int height)
{
    round_thin_border_draw(fi, mri, left, top, width, height, MPEN_SHADOW,
                           MPEN_HALFSHADOW, MPEN_BACKGROUND, MPEN_HALFSHINE,
                           MPEN_SHINE);
}

/**************************************************************************
 9 : FST_GRAY_BORDER
**************************************************************************/
/**
 * Gray border frame (raised) - FST_GRAY_BORDER
 * Uses half-tone pens for subtle effect
 */
static void frame_gray_border_up_draw(struct dt_frame_image *fi,
                                      struct MUI_RenderInfo *mri, int gl,
                                      int gt, int gw, int gh, int left, int top,
                                      int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_HALFSHINE,
                    MPEN_HALFSHADOW);
}

/**************************************************************************

**************************************************************************/
/**
 * Gray border frame (recessed) - FST_GRAY_BORDER
 * Uses half-tone pens for subtle effect
 */
static void frame_gray_border_down_draw(struct dt_frame_image *fi,
                                        struct MUI_RenderInfo *mri, int gl,
                                        int gt, int gw, int gh, int left,
                                        int top, int width, int height)
{
    thinborder_draw(fi, mri, left, top, width, height, MPEN_HALFSHADOW,
                    MPEN_HALFSHINE);
}

/**************************************************************************
 A : FST_SEMIROUND_BEVEL
**************************************************************************/
static void semiround_bevel_draw(struct dt_frame_image *fi,
                                 struct MUI_RenderInfo *mri, int left, int top,
                                 int width, int height, MPen pen1, MPen pen2,
                                 MPen pen3, MPen pen4, MPen pen5)
{
    struct RastPort *rp = mri->mri_RastPort;

    button_draw(fi, mri, left, top, width, height, pen1, pen5);
    button_draw(fi, mri, left + 1, top + 1, width - 2, height - 2, pen2, pen4);

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
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    semiround_bevel_draw(fi, mri, left, top, width, height, MPEN_SHINE,
                         MPEN_HALFSHINE, MPEN_BACKGROUND, MPEN_HALFSHADOW,
                         MPEN_SHADOW);
}

/**************************************************************************

**************************************************************************/
static void frame_semiround_bevel_down_draw(struct dt_frame_image *fi,
        struct MUI_RenderInfo *mri, int gl,
        int gt, int gw, int gh, int left,
        int top, int width, int height)
{
    semiround_bevel_draw(fi, mri, left, top, width, height, MPEN_SHADOW,
                         MPEN_HALFSHADOW, MPEN_BACKGROUND, MPEN_HALFSHINE,
                         MPEN_SHINE);
}

/**************************************************************************
 Modern frame styles - contemporary flat and minimal designs
**************************************************************************/

/**
 * Draw a smooth circle corner using Bresenham's circle algorithm
 * @param rp RastPort to draw on
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Circle radius
 * @param pen Pen color to use
 * @param quadrant Which quadrant to draw:
 *                 1 = Northeast (top-right corner)
 *                 2 = Northwest (top-left corner)
 *                 4 = Southwest (bottom-left corner)
 *                 8 = Southeast (bottom-right corner)
 */
static void draw_smooth_corner(struct RastPort *rp, int cx, int cy, int radius,
                               ULONG pen, int quadrant)
{
    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    SetAPen(rp, pen);

    while (y >= x) {
        /* Draw pixels in selected quadrant only */
        if (quadrant & 1) { /* Northeast (top-right corner) */
            WritePixel(rp, cx + x, cy - y);
            WritePixel(rp, cx + y, cy - x);
        }
        if (quadrant & 2) { /* Northwest (top-left corner) */
            WritePixel(rp, cx - x, cy - y);
            WritePixel(rp, cx - y, cy - x);
        }
        if (quadrant & 4) { /* Southwest (bottom-left corner) */
            WritePixel(rp, cx - x, cy + y);
            WritePixel(rp, cx - y, cy + x);
        }
        if (quadrant & 8) { /* Southeast (bottom-right corner) */
            WritePixel(rp, cx + x, cy + y);
            WritePixel(rp, cx + y, cy + x);
        }

        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        x++;
    }
}

/**
 * Helper function to fill the connection pixels between circle and straight
 * edges This ensures no gaps appear between the curved corners and straight
 * lines
 */
static void fill_corner_connections(struct RastPort *rp, int left, int top,
                                    int width, int height, int radius,
                                    ULONG light_pen, ULONG dark_pen)
{
    /* Fill the exact pixels where curves should connect to straight lines */

    /* Top-left corner connections */
    SetAPen(rp, light_pen);
    WritePixel(rp, left + radius, top); /* Top edge connection */
    WritePixel(rp, left, top + radius); /* Left edge connection */

    /* Top-right corner connections */
    WritePixel(rp, left + width - radius - 1, top); /* Top edge connection */
    SetAPen(rp, dark_pen);
    WritePixel(rp, left + width - 1, top + radius); /* Right edge connection */

    /* Bottom-left corner connections */
    SetAPen(rp, light_pen);
    WritePixel(rp, left, top + height - radius - 1); /* Left edge connection */
    SetAPen(rp, dark_pen);
    WritePixel(rp, left + radius, top + height - 1); /* Bottom edge connection */

    /* Bottom-right corner connections */
    WritePixel(rp, left + width - radius - 1, top + height - 1); /* Bottom edge */
    WritePixel(rp, left + width - 1, top + height - radius - 1); /* Right edge */
}

/**
 * D: Rounded frame
 * Parameters moved from globals to struct dt_frame_image members
 */
/* Frame parameters are now part of the dt_frame_image struct */

static void frame_rounded_draw(struct dt_frame_image *fi,
                               struct MUI_RenderInfo *mri, int gl, int gt,
                               int gw, int gh, int left, int top, int width,
                               int height, MPen light_pen, MPen dark_pen)
{
    struct RastPort *rp = mri->mri_RastPort;

    /* Use the radius from the frame image struct */
    int radius = fi ? fi->border_radius : 8;

    if (width <= radius * 2 + 4 || height <= radius * 2 + 4)
        return; /* Need space for radius plus some margin */

    /* Use the frame width from the frame image struct */
    int frame_width = fi ? fi->frame_width : 1;

    SetAPen(rp, mri->mri_Pens[light_pen]);
    /* Top edge: from end of left curve to start of right curve */
    RectFill(rp, left + radius, top, left + width - radius - 1,
             top + frame_width - 1);
    /* Left edge: from end of top curve to start of bottom curve */
    RectFill(rp, left, top + radius, left + frame_width - 1,
             top + height - radius - 1);

    SetAPen(rp, mri->mri_Pens[dark_pen]);
    /* Bottom edge: from end of left curve to start of right curve */
    RectFill(rp, left + radius, top + height - frame_width,
             left + width - radius - 1, top + height - 1);
    /* Right edge: from end of top curve to start of bottom curve */
    RectFill(rp, left + width - frame_width, top + radius, left + width - 1,
             top + height - radius - 1);

    if (frame_width < 2) {
        /* Top-left corner: center at (left+radius, top+radius) */
        draw_smooth_corner(rp, left + radius, top + radius, radius,
                           mri->mri_Pens[light_pen], 2);

        /* Top-right corner: center at (left+width-radius-1, top+radius) */
        draw_smooth_corner(rp, left + width - radius - 1, top + radius, radius,
                           mri->mri_Pens[light_pen], 1);

        /* Bottom-left corner: center at (left+radius, top+height-radius-1) */
        draw_smooth_corner(rp, left + radius, top + height - radius - 1, radius,
                           mri->mri_Pens[dark_pen], 4);

        /* Bottom-right corner: center at (left+width-radius-1, top+height-radius-1)
         */
        draw_smooth_corner(rp, left + width - radius - 1, top + height - radius - 1,
                           radius, mri->mri_Pens[dark_pen], 8);
    } else {
        /* Draw solid rounded corners by filling the corner areas pixel by pixel */
        int x, y;

        for (y = 0; y < radius; y++) {
            for (x = 0; x < radius; x++) {
                int dx = radius - x - 1;
                int dy = radius - y - 1;
                int distance_sq = dx * dx + dy * dy;
                int outer_radius_sq = radius * radius;
                int inner_radius = radius - frame_width;
                int inner_radius_sq = inner_radius * inner_radius;

                /* Only draw if pixel is in the border area (between inner and outer
                 * radius) */
                if (distance_sq < outer_radius_sq &&
                        (inner_radius <= 0 || distance_sq >= inner_radius_sq)) {
                    /* Top-left corner */
                    SetAPen(rp, mri->mri_Pens[light_pen]);
                    WritePixel(rp, left + x, top + y);

                    /* Top-right corner - mirror x coordinate */
                    WritePixel(rp, left + width - x - 1, top + y);

                    /* Bottom-left corner - mirror y coordinate */
                    SetAPen(rp, mri->mri_Pens[dark_pen]);
                    WritePixel(rp, left + x, top + height - y - 1);

                    /* Bottom-right corner - mirror both x and y coordinates */
                    WritePixel(rp, left + width - x - 1, top + height - y - 1);
                }
            }
        }
    }
    // 1,
    //                    radius - 1, mri->mri_Pens[MPEN_HALFSHADOW], 8);

    /* Ensure perfect connections between curves and straight lines */
    fill_corner_connections(rp, left, top, width, height, radius,
                            mri->mri_Pens[light_pen], mri->mri_Pens[dark_pen]);
}

/**
 * Rounded frame (raised state)
 */
static void frame_rounded_up_draw(struct dt_frame_image *fi,
                                  struct MUI_RenderInfo *mri, int gl, int gt,
                                  int gw, int gh, int left, int top, int width,
                                  int height)
{
    frame_rounded_draw(fi, mri, gl, gt, gw, gh, left, top, width, height,
                       MPEN_HALFSHADOW, MPEN_HALFSHADOW);
}

/**
 * Rounded frame (pressed state)
 */
static void frame_rounded_down_draw(struct dt_frame_image *fi,
                                    struct MUI_RenderInfo *mri, int gl, int gt,
                                    int gw, int gh, int left, int top,
                                    int width, int height)
{
    frame_rounded_draw(fi, mri, gl, gt, gw, gh, left, top, width, height,
                       MPEN_HALFSHADOW, MPEN_HALFSHADOW);
}

/**************************************************************************
 hold builtin frames.
**************************************************************************/
static const struct ZuneFrameGfx __builtinFrameGfx[] = {
    /* type 0 : FST_NONE */
    {frame_none_draw, 0, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_none_draw, 0, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    /* monochrome border */
    /* 1 : FST_RECT */
    {frame_white_rect_draw, 0, 1, 1, 1, 1, NULL, FALSE, 1, 0},
    {frame_black_rect_draw, 0, 1, 1, 1, 1, NULL, FALSE, 1, 0},

    /* clean 3D look */
    /* 2 : FST_BEVEL */
    {frame_bevelled_draw, 0, 1, 1, 1, 1, NULL, FALSE, 1, 0},
    {frame_recessed_draw, 0, 1, 1, 1, 1, NULL, FALSE, 1, 0},

    /* thin relief border */
    /* 3 : FST_THIN_BORDER */
    {frame_thin_border_up_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 0},
    {frame_thin_border_down_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 0},

    /* thick relief border */
    /* 4 : FST_THICK_BORDER */
    {frame_thick_border_up_draw, 0, 3, 3, 3, 3, NULL, FALSE, 3, 0},
    {frame_thick_border_down_draw, 0, 3, 3, 3, 3, NULL, FALSE, 3, 0},

    /* rounded bevel */
    /* 5 : FST_ROUND_BEVEL */
    {frame_round_bevel_up_draw, 0, 4, 4, 1, 1, NULL, FALSE, 2, 2},
    {frame_round_bevel_down_draw, 0, 4, 4, 1, 1, NULL, FALSE, 2, 2},

    /* zin31/xen look */
    /* 6 : FST_WIN_BEVEL */
    {frame_border_button_up_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 0},
    {frame_border_button_down_draw, 0, 3, 1, 3, 1, NULL, FALSE, 2, 0},

    /* rounded thick border */
    /* 7 : FST_ROUND_THICK_BORDER */
    {frame_round_thick_border_up_draw, 0, 4, 4, 4, 4, NULL, FALSE, 4, 3},
    {frame_round_thick_border_down_draw, 0, 4, 4, 4, 4, NULL, FALSE, 4, 3},

    /* rounded thin border */
    /* 8 : FST_ROUND_THIN_BORDER */
    {frame_round_thin_border_up_draw, 0, 4, 4, 4, 4, NULL, FALSE, 2, 2},
    {frame_round_thin_border_down_draw, 0, 4, 4, 4, 4, NULL, FALSE, 2, 2},

    /* strange gray border */
    /* 9 : FST_GRAY_BORDER */
    {frame_gray_border_up_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 0},
    {frame_gray_border_down_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 0},

    /* semi rounded bevel */
    /* A : FST_SEMIROUND_BEVEL */
    {frame_semiround_bevel_up_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 1},
    {frame_semiround_bevel_down_draw, 0, 2, 2, 2, 2, NULL, FALSE, 2, 1},

    /* B : FST_ROUNDED */
    {frame_rounded_up_draw, 0, 2, 2, 2, 2, NULL, FALSE, 1, 8},
    {frame_rounded_down_draw, 0, 2, 2, 2, 2, NULL, FALSE, 1, 8},

    /* custom frames */

    {frame_custom_up, 1, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 1, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 2, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 2, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 3, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 3, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 4, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 4, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 5, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 5, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 6, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 6, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 7, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 7, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 8, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 8, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 9, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 9, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 10, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 10, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 11, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 11, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 12, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 12, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 13, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 13, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 14, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 14, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 15, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 15, 0, 0, 0, 0, NULL, FALSE, 0, 0},

    {frame_custom_up, 16, 0, 0, 0, 0, NULL, FALSE, 0, 0},
    {frame_custom_down, 16, 0, 0, 0, 0, NULL, FALSE, 0, 0},

};

/**************************************************************************

**************************************************************************/
const struct ZuneFrameGfx *
zune_zframe_get(Object *obj, const struct MUI_FrameSpec_intern *frameSpec)
{
    struct dt_frame_image *fi = NULL;

    if (frameSpec->type >= FST_CUSTOM1) {
        struct MUI_RenderInfo *mri = muiRenderInfo(obj);
        if (!(fi = mri->mri_FrameImage[frameSpec->type - FST_CUSTOM1]))
            return &__builtinFrameGfx[2 * FST_RECT];
    }

    if (frameSpec->type >= FST_COUNT)
        return &__builtinFrameGfx[2 * FST_RECT];

    return &__builtinFrameGfx[2 * frameSpec->type + frameSpec->state];

#if 0
    frame->customframe = NULL;

    if ((fi != NULL) && (frame != NULL)) {
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

const struct ZuneFrameGfx *zune_zframe_get_with_state(
    Object *obj, const struct MUI_FrameSpec_intern *frameSpec, UWORD state)
{
    struct dt_frame_image *fi = NULL;

    if (frameSpec->type >= FST_CUSTOM1) {
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
    snprintf(tmpSpec, 3, "%lx", (ULONG)intern->state);
    spec[1] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (ULONG)intern->innerLeft);
    spec[2] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (ULONG)intern->innerRight);
    spec[3] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (ULONG)intern->innerTop);
    spec[4] = tmpSpec[0];
    snprintf(tmpSpec, 3, "%lx", (ULONG)intern->innerBottom);
    spec[5] = tmpSpec[0];

    /* Add optional border_radius and border_width fields */
    if (intern->border_radius != 0 || intern->border_width != 0) {
        snprintf(tmpSpec, 3, "%lx", (ULONG)intern->border_radius);
        spec[6] = tmpSpec[0];
        snprintf(tmpSpec, 3, "%lx", (ULONG)intern->border_width);
        spec[7] = tmpSpec[0];
        spec[8] = '\0';
    } else {
        spec[6] = '\0';
    }

    return TRUE;
}

/* Helper function to create a dt_frame_image with proper values for drawing */
struct dt_frame_image *
zune_frame_prepare_for_drawing(const struct ZuneFrameGfx *zframe,
                               const struct MUI_FrameSpec_intern *framespec,
                               struct dt_frame_image *temp_storage)
{

    /* If there's already a custom frame, use it */
    if (zframe->customframe) {
        return zframe->customframe;
    }

    /* Create a temporary frame image with proper values */
    memset(temp_storage, 0, sizeof(struct dt_frame_image));

    /* Set border radius and width from framespec if available,
     * the margins are already calculated at this point */
    if (framespec && framespec->type == FST_ROUNDED) {
        temp_storage->border_radius = framespec->border_radius;
        temp_storage->frame_width = framespec->border_width;
    } else {
        /* Use static defaults from ZuneFrameGfx */
        temp_storage->border_radius = zframe->border_radius;
        temp_storage->frame_width = zframe->border_width;
    }

    return temp_storage;
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
    int specLen;

    if (!intern || !spec)
        return FALSE;

    specLen = strlen(spec);
    /* Validate spec string length - minimum 6 chars, maximum 8 chars */
    if (specLen < 6 || specLen > 8)
        return FALSE;

    val = xhexasciichar_to_int(spec[0]);
    if (val == -1 || val >= FST_COUNT)
        return FALSE;
    intern->type = val;

    val = xhexasciichar_to_int(spec[1]);
    if (val == -1 || val > 1)
        return FALSE;
    intern->state = val;

    val = xhexasciichar_to_int(spec[2]);
    if (val == -1 || val > 15)
        return FALSE;
    intern->innerLeft = val;

    val = xhexasciichar_to_int(spec[3]);
    if (val == -1 || val > 15)
        return FALSE;
    intern->innerRight = val;

    val = xhexasciichar_to_int(spec[4]);
    if (val == -1 || val > 15)
        return FALSE;
    intern->innerTop = val;

    val = xhexasciichar_to_int(spec[5]);
    if (val == -1 || val > 15)
        return FALSE;
    intern->innerBottom = val;

    /* Initialize optional fields to 0 */
    intern->border_radius = 0;
    intern->border_width = 0;

    /* Parse optional border_radius and border_width if present */
    if (specLen >= 7) {
        val = xhexasciichar_to_int(spec[6]);
        if (val == -1 || val > 15)
            return FALSE;
        intern->border_radius = val;
    }

    if (specLen >= 8) {
        val = xhexasciichar_to_int(spec[7]);
        if (val == -1 || val > 15)
            return FALSE;
        intern->border_width = val;
    }

    return TRUE;
}

/**************************************************************************
 Frame support for rounded corners
**************************************************************************/

/**
 * Query frame information including border radius and width
 * @param obj MUI object
 * @param frameSpec Frame specification
 * @param characteristics Output structure to fill with info
 * @return TRUE if successful, FALSE on error
 */
BOOL zune_frame_get_characteristics(
    Object *obj, const struct MUI_FrameSpec_intern *frameSpec,
    struct MUI_FrameCharacteristics *characteristics)
{
    if (!obj || !frameSpec || !characteristics)
        return FALSE;

    /* Fill in frame information */
    characteristics->border_width = frameSpec->border_width;
    characteristics->border_radius = frameSpec->border_radius;
    characteristics->has_rounded_corners = (frameSpec->border_radius > 0);

    return TRUE;
}

/**
 * Create a clipping region for rounded frames
 * @param left Left coordinate of frame
 * @param top Top coordinate of frame
 * @param width Width of frame
 * @param height Height of frame
 * @param clipinfo Output Frame clipping information
 * @return Clipping region or NULL on error
 */
struct Region *zune_frame_create_clip_region(int left, int top, int width,
        int height, UWORD border_radius)
{
    struct Region *region;
    struct Rectangle rect;

    if (width <= 0 || height <= 0)
        return NULL;

    /* For non-rounded frames, return full rectangular region */
    if (border_radius == 0) {
        region = NewRegion();
        if (region) {
            rect.MinX = left;
            rect.MinY = top;
            rect.MaxX = left + width - 1;
            rect.MaxY = top + height - 1;
            OrRectRegion(region, &rect);
        }
        return region;
    }

    /* Create clipping region for rounded corners */
    region = NewRegion();
    if (!region)
        return NULL;

    int radius = border_radius;

    /* Ensure radius doesn't exceed frame dimensions */
    if (radius * 2 > width)
        radius = width / 2;
    if (radius * 2 > height)
        radius = height / 2;

    /* Add main rectangular area (excluding corner areas) */

    /* Top rectangle (above corners) */
    if (radius < height / 2) {
        rect.MinX = left + radius;
        rect.MinY = top;
        rect.MaxX = left + width - radius - 1;
        rect.MaxY = top + radius - 1;
        if (rect.MinX <= rect.MaxX && rect.MinY <= rect.MaxY)
            OrRectRegion(region, &rect);
    }

    /* Middle rectangle (full width between corners) */
    rect.MinX = left;
    rect.MinY = top + radius;
    rect.MaxX = left + width - 1;
    rect.MaxY = top + height - radius - 1;
    if (rect.MinX <= rect.MaxX && rect.MinY <= rect.MaxY)
        OrRectRegion(region, &rect);

    /* Bottom rectangle (below corners) */
    if (radius < height / 2) {
        rect.MinX = left + radius;
        rect.MinY = top + height - radius;
        rect.MaxX = left + width - radius - 1;
        rect.MaxY = top + height - 1;
        if (rect.MinX <= rect.MaxX && rect.MinY <= rect.MaxY)
            OrRectRegion(region, &rect);
    }

    /* Add rounded corner pixels using circle algorithm */
    int x, y;
    int cx, cy;

    /* For each corner, calculate which pixels are inside the rounded area */
    for (y = 0; y < radius; y++) {
        for (x = 0; x < radius; x++) {
            /* Check if pixel is inside circle using distance formula */
            int dx = radius - x - 1;
            int dy = radius - y - 1;
            /* Use a more conservative threshold to prevent background leaking */
            int threshold = (radius > 2) ? (radius - 2) * (radius - 2) : 1;
            if (dx * dx + dy * dy < threshold) {
                /* Top-left corner */
                rect.MinX = left + x;
                rect.MinY = top + y;
                rect.MaxX = left + x;
                rect.MaxY = top + y;
                OrRectRegion(region, &rect);

                /* Top-right corner */
                rect.MinX = left + width - x - 1;
                rect.MinY = top + y;
                rect.MaxX = left + width - x - 1;
                rect.MaxY = top + y;
                OrRectRegion(region, &rect);

                /* Bottom-left corner */
                rect.MinX = left + x;
                rect.MinY = top + height - y - 1;
                rect.MaxX = left + x;
                rect.MaxY = top + height - y - 1;
                OrRectRegion(region, &rect);

                /* Bottom-right corner */
                rect.MinX = left + width - x - 1;
                rect.MinY = top + height - y - 1;
                rect.MaxX = left + width - x - 1;
                rect.MaxY = top + height - y - 1;
                OrRectRegion(region, &rect);
            }
        }
    }

    return region;
}
