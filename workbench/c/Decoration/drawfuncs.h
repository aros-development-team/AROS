/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef DRAWFUNCS_H
#define DRAWFUNCS_H

#include <graphics/rastport.h>

void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle);
void DrawTileToRP(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h);
void WriteAlphaPixelArray(struct NewImage *src, struct NewLUT8Image *dst, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h);
LONG WriteTiledImageTitle(BOOL fill, struct Window *win, struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);
/* TODO: remove need for windecor_data */
void DrawAlphaStateImageToRP(struct windecor_data *data, struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp, BOOL multiple);
/* TODO: remove need for windecor_data */
void ShadeLine(LONG pen, BOOL tc, struct windecor_data *d, struct RastPort *rp, struct NewImage *ni, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1);
void FillPixelArrayGradientDelta(LONG pen, BOOL tc, struct RastPort *rp, LONG xt, LONG yt, LONG xb, LONG yb, LONG xp, LONG yp, LONG w, LONG h, ULONG start_rgb, ULONG end_rgb, LONG angle, LONG dx, LONG dy);
void DrawTileToRPRoot(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h);
LONG WriteTiledImageVertical(struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);
LONG WriteTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, LONG sx, LONG sy, LONG sw, LONG sh, LONG xp, LONG yp, LONG dw, LONG dh);
#endif
